//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"

static const float MaxAngleOffsetDeg = 0.25f;
static const uint  SampleCount = 16;

//***********************************************************************
//*****------ TraceRay wrappers for radiance and AO rays. -------********
//***********************************************************************

// Trace a radiance ray into the scene.
void TraceRadianceRay(in Ray ray, inout ShadowPayload payload)
{
    // We trace only one primary ray per pixel for shadows, so recursion depth does not need to be tracked.

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = 0;
    rayDesc.TMax = MaxPrimaryRayLength;

    TraceRay(
        Scene, //g_scene,
        RAY_FLAG_NONE, //RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF, //TraceRayParameters::InstanceMask,
        RayType::Radiance, //TraceRayParameters::HitGroup::Offset[RayType::Radiance],
        0, //TraceRayParameters::HitGroup::GeometryStride,
        RayType::Radiance, //TraceRayParameters::MissShader::Offset[RayType::Radiance],
        rayDesc,
        payload);
}

// Trace a shadow ray and return true if it hits any geometry.
void TraceShadowRayAndReportIfHit(in Ray ray, inout ShadowPayload payload)
{
    payload.occlusion = 1;  // Set to full occlusion, and shadow ray miss shader, if called, will set to zero.

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    // Set TMin to a zero value to avoid aliasing artifcats along contact areas.
    // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0; //SurfaceEpsilon;
    rayDesc.TMax = MaxPrimaryRayLength;

    TraceRay(
        Scene, //g_scene,
        //RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // ~skip closest hit shaders,
        0xFF, //TraceRayParameters::InstanceMask,
        RayType::Occlusion, //TraceRayParameters::HitGroup::Offset[RayType::Shadow],
        0,//MeshGeometries::Count, //TraceRayParameters::HitGroup::GeometryStride,
        //TriangleMeshes::Count, //TraceRayParameters::HitGroup::GeometryStride,
        RayType::Occlusion, //TraceRayParameters::MissShader::Offset[RayType::Shadow],
        rayDesc,
        payload);
        //shadowPayload);
}

[shader("raygeneration")]
void RaygenShader()
{
    // Get the location within the dispatched 2D grid of work items
    // (often maps to pixels, so this could represent a pixel coordinate).
    const uint2 launchIndex    = DispatchRaysIndex().xy; // Get screen pixel coords.
    const float3 cameraPos     = frameCB.cameraPos.xyz;
    const float4x4 invViewProj = frameCB.invViewProj;

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    const Ray ray = GenerateCameraRay(launchIndex, cameraPos, invViewProj);
    const Ray ddx = GenerateCameraRay(uint2(launchIndex.x + 1, launchIndex.y), cameraPos, invViewProj);
    const Ray ddy = GenerateCameraRay(uint2(launchIndex.x, launchIndex.y + 1), cameraPos, invViewProj);

    ShadowPayload payload;
    //payload.occlusion = 1;
    //payload.isShadowRayMiss= false;  // Set to false, and shadow ray miss shader, if called, will set to true.
    payload.ddxRay = ddx;
    payload.ddyRay = ddy;

    TraceRadianceRay(ray, payload);

    RWTexture2D<float> Output = ResourceDescriptorHeap[SrvUAVs::ShadowBuffer0Uav];

    // Shadow visibility buffer stores a scalar value between 0 (full occlusion) and 1 (full visibility).
    Output[launchIndex] = 1.f - payload.occlusion;
    //ShadowOutput[launchIndex] = (float)payload.isShadowRayMiss;
    //AmbientAccessOutput[launchIndex] = saturate(pow(payload.ambientAccess, 6.f));
}

[shader("closesthit")]
void ClosestHitCubeShader(inout ShadowPayload payload, in Attributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()]; // Index into structured buffer with DXR intrinsics.
    //StructuredBuffer<GeometryData> Geometries = ResourceDescriptorHeap[SrvUAVs::GeometryBufferSrv];
    //GeometryData geom = Geometries[InstanceID()]; // Index into structured buffer with DXR intrinsic.

    StructuredBuffer<VertexCube> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, PrimitiveIndex(), Indices);
    //const uint3 indices = isIndex16 ? Load3x16BitIndices(baseIndex, Indices) : Indices.Load3(baseIndex);
    //const uint3 indices = Load3x16BitIndices(baseIndex, Indices);

    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] = {
        mul(Vertices[indices[0]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[1]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[2]].normal, (float3x3)ObjectToWorld4x3()),
    };
    // Retrieve corresponding texcoords for the triangle vertices.
    //float2 vertexTexCoords[3] = {
    //	Vertices[indices[0]].texCoord,
    //	Vertices[indices[1]].texCoord,
    //	Vertices[indices[2]].texCoord,
    //};

    //float2 uv = HitAttribute(vertexTexCoords, attr);

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.
    const float3 worldPos    = HitWorldPosition();
    const float3 worldNormal = vertexNormals[0];    // Vertex normals are identical, so no interpolation required.
    //const float3 hitNormal = mul(vertexNormals[0], (float3x3)ObjectToWorld4x3());

    const float3 L = -frameCB.lightDir; // Light vector ("to light" opposite of light's direction).

    // Helper rays
    const Ray ddxRay = payload.ddxRay;
    const Ray ddyRay = payload.ddyRay;

    // Intersect helper rays
    const float3 xOffsetPoint = RayPlaneIntersection(worldPos, worldNormal, ddxRay);
    const float3 yOffsetPoint = RayPlaneIntersection(worldPos, worldNormal, ddyRay);

    // Trace a shadow ray.
    // Calculate a ray origin offset to avoid self-intersection.
    // Shadow terminator correction is not required for cubes.
    const float3 offsetOrigin = GetOffsetRayOrigin(worldPos, worldNormal);

    // Sample the sun's visibility from the hitpoint using a offset light direction vectors.

    // Random number seed generation.
    // Where is this thread's ray on screen?
    const uint2 launchIndex = DispatchRaysIndex().xy;
    const uint2 launchDim   = DispatchRaysDimensions().xy;

    // Initialize a random seed, per-pixel, based on a screen position and temporally varying count
    uint randSeed = InitRand(launchIndex.x + launchIndex.y * launchDim.x, frameCB.frameCount /*, 16 */);

    float occlusionSum = 0; // Start value.

    [unroll]
    for (uint i = 0; i < SampleCount; ++i)
    {
        // Get a direction vector to the light, offset by a small random angle.
        const float3 offsetL = UniformSampleCone(randSeed, L, MaxAngleOffsetDeg);
        //const float3 offsetL = GetOffsetShadowRayDirection(randSeed, L);

        const Ray shadowRay    = { offsetOrigin, offsetL };
        const Ray ddxShadowRay = { xOffsetPoint, offsetL };
        const Ray ddyShadowRay = { yOffsetPoint, offsetL };

        // Update payload with auxilliary shadow rays.
        payload.ddxRay = ddxShadowRay;
        payload.ddyRay = ddyShadowRay;

        TraceShadowRayAndReportIfHit(shadowRay, payload);

        occlusionSum += payload.occlusion;
    }

    // Get average occlusion value and set it into the payload.
    occlusionSum /= (float)SampleCount;
    payload.occlusion = occlusionSum;
}

[shader("closesthit")]
void ClosestHitShader(inout ShadowPayload payload, Attributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()+GeometryIndex()]; // Index into structured buffer with DXR intrinsics.
    //StructuredBuffer<GeometryData> Geometries = ResourceDescriptorHeap[SrvUAVs::GeometryBufferSrv];
    //GeometryData geom = Geometries[InstanceID()]; // Index into structured buffer with DXR intrinsic.

    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, PrimitiveIndex(), Indices);
    //const uint3 indices = isIndex16 ? Load3x16BitIndices(baseIndex, Indices) : Indices.Load3(baseIndex);
    //const uint3 indices = Load3x16BitIndices(baseIndex, Indices);

    // Retrieve corresponding vertex positions for the triangle vertices.
    const float3 vertexPositions[3] = {
        mul(float4(Vertices[indices[0]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[1]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[2]].position, 1), ObjectToWorld4x3()),
    };
    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] = {
        mul(Vertices[indices[0]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[1]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[2]].normal, (float3x3)ObjectToWorld4x3()),
    };

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.
    const float3 worldPos    = HitWorldPosition();
    const float3 worldNormal = HitAttribute(vertexNormals, attr);    // Get interpolated normal at the hit position.
    //float3 hitNormal = HitAttribute(vertexNormals, attr);    // Get interpolated normal at the hit position.
    //hitNormal = mul(hitNormal, (float3x3)ObjectToWorld4x3());

    const float3 L = -frameCB.lightDir; // Light vector ("to light" opposite of light's direction).

    // Helper rays
    const Ray ddxRay = payload.ddxRay;
    const Ray ddyRay = payload.ddyRay;

    // Intersect helper rays
    const float3 xOffsetPoint = RayPlaneIntersection(worldPos, worldNormal, ddxRay);
    const float3 yOffsetPoint = RayPlaneIntersection(worldPos, worldNormal, ddyRay);

    // Trace a shadow ray.
    // To eliminate shadow terminator artifacts, we calculate a new point for the shadow ray origin.
    const float3 shadowOrigin  = GetOffsetShadowRayOrigin(worldPos, vertexPositions, vertexNormals, attr);
    // Also calculate a ray origin offset to avoid self-intersection.
    const float3 offsetOrigin = GetOffsetRayOrigin(shadowOrigin, worldNormal);

    // Sample the sun's visibility from the hitpoint using a offset light direction vectors.

    // Random number seed generation.
    // Where is this thread's ray on screen?
    const uint2 launchIndex = DispatchRaysIndex().xy;
    const uint2 launchDim   = DispatchRaysDimensions().xy;

    // Initialize a random seed, per-pixel, based on a screen position and temporally varying count.
    uint randSeed = InitRand(launchIndex.x + launchIndex.y * launchDim.x, frameCB.frameCount /*, 16 */);

    float occlusionSum = 0; // Start value.

    [unroll]
    for (uint i = 0; i < SampleCount; ++i)
    {
        // Get a direction vector to the light, offset by a small random angle.
        const float3 offsetL = UniformSampleCone(randSeed, L, MaxAngleOffsetDeg);
        //const float3 offsetL = GetOffsetShadowRayDirection(randSeed, L);

        const Ray shadowRay    = { offsetOrigin, offsetL };
        const Ray ddxShadowRay = { xOffsetPoint, offsetL };
        const Ray ddyShadowRay = { yOffsetPoint, offsetL };

        // Update payload with auxilliary shadow rays.
        payload.ddxRay = ddxShadowRay;
        payload.ddyRay = ddyShadowRay;

        TraceShadowRayAndReportIfHit(shadowRay, payload);

        occlusionSum += payload.occlusion;
    }

    // Get average occlusion value and set it into the payload.
    occlusionSum /= (float)SampleCount;
    payload.occlusion = occlusionSum;
}

//***************************************************************************
//*****************------ Any hit shaders-------************************
//***************************************************************************

// NB: Any hit shaders will be ignored if D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE flag is set in the application,
// unless overriden by D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE, or ray flag RAY_FLAG_FORCE_NON_OPAQUE.
//[shader("anyhit")]
//void AnyHitCubeShader(inout NormalDepthRayPayload payload, Attributes attr)
//{
//	// Unused.
//}

[shader("anyhit")]
void AnyHitShader(inout ShadowPayload payload, Attributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()+GeometryIndex()]; // Index into structured buffer with DXR intrinsic.

    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, PrimitiveIndex(), Indices);
    //const uint3 indices = isIndex16 ? Load3x16BitIndices(baseIndex, Indices) : Indices.Load3(baseIndex);

    // Retrieve corresponding vertex positions for the triangle vertices.
    // Texture filtering with ray differentials was solved by transforming vertex positions to world space.
    const float3 vertexPositions[3] = {
        mul(float4(Vertices[indices[0]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[1]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[2]].position, 1), ObjectToWorld4x3()),
    };
    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] = {
        mul(Vertices[indices[0]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[1]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[2]].normal, (float3x3)ObjectToWorld4x3()),
    };
    // Retrieve corresponding texcoords for the triangle vertices.
    const float2 vertexTexCoords[3] =
    {
        Vertices[indices[0]].texCoord,
        Vertices[indices[1]].texCoord,
        Vertices[indices[2]].texCoord,
    };
                        
    // Get hit position.
    const float3 worldPos    = HitWorldPosition();
    const float3 worldNormal = HitAttribute(vertexNormals, attr);    // Get interpolated normal at the hit position.
    const float2 uv          = HitAttribute(vertexTexCoords, attr);
    //int2 uvScaled = floor(texCoord * 1024.f);
    //hitNormal = mul(hitNormal, (float3x3) ObjectToWorld4x3());

    // Sample the material texture.
    //---------------------------------------------------------------------------------------------
    // Compute partial derivatives of UV coordinates:
    //
    //  1) Construct a plane from the hit triangle
    //  2) Intersect two helper rays with the plane:  one to the right and one down
    //  3) Compute barycentric coordinates of the two hit points
    //  4) Reconstruct the UV coordinates at the hit points
    //  5) Take the difference in UV coordinates as the partial derivatives X and Y

    // Normal for plane
    //float3 planeNormal = normalize(cross(vertexPositions[2] - vertexPositions[0], vertexPositions[1] - vertexPositions[0]));
    //planeNormal = normalize(mul(planeNormal, (float3x3)ObjectToWorld4x3()));

    // Helper rays
    const Ray ddxRay = payload.ddxRay;
    const Ray ddyRay = payload.ddyRay;
    //uint2 threadID = DispatchRaysIndex().xy;
    //Ray ddxRay = GenerateCameraRay(uint2(threadID.x + 1, threadID.y), frameCB.cameraPos.xyz, frameCB.invViewProj);
    //Ray ddyRay = GenerateCameraRay(uint2(threadID.x, threadID.y + 1), frameCB.cameraPos.xyz, frameCB.invViewProj);

    // Intersect helper rays
    const float3 xOffsetPoint = RayPlaneIntersection(worldPos, worldNormal, ddxRay);
    const float3 yOffsetPoint = RayPlaneIntersection(worldPos, worldNormal, ddyRay);

    const PartialDerivatives pd = GetPartialDerivatives(xOffsetPoint, yOffsetPoint, vertexPositions, vertexTexCoords, uv);	

    // Compute barycentrics 
    //float3 baryX = BarycentricCoordinates(xOffsetPoint, vertexPositions[0], vertexPositions[1], vertexPositions[2]);
    //float3 baryY = BarycentricCoordinates(yOffsetPoint, vertexPositions[0], vertexPositions[1], vertexPositions[2]);

    // Compute UVs and take the difference
    //float3x2 uvMatrix = float3x2(vertexTexCoords);
    //float2 ddxUV = mul(baryX, uvMatrix) - uv;
    //float2 ddyUV = mul(baryY, uvMatrix) - uv;

    Texture2D<float4> AlbedoMap = ResourceDescriptorHeap[inst.material.albedoTexIndex];
    const float texAlpha = AlbedoMap.SampleGrad(AnisoClamp, uv, pd.ddxUV, pd.ddyUV).a;
    //float texAlpha = AlbedoMap.SampleGrad(AnisoClamp, uv, ddxUV, ddyUV).a;
    //float texAlpha = AlbedoMap.SampleLevel(AnisoClamp, uv, 0).a;

    if (texAlpha < 0.1f)
        IgnoreHit();
}

//[shader("anyhit")]
//void AnyHitShader_Occlusion(inout NormalDepthRayPayload payload, Attributes attr){} // Not used

//***************************************************************************
//**********************------ Miss shaders -------**************************
//***************************************************************************

[shader("miss")]
void MissShader_CameraRay(inout ShadowPayload payload)
{
    payload.occlusion = 0;
    //payload.isShadowRayMiss = true;
}

[shader("miss")]
void MissShader_Occlusion(inout ShadowPayload payload)
{
    payload.occlusion = 0;
    //payload.isShadowRayMiss = true;
}
