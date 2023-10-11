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

//#ifndef RAYTRACING_AO_HLSL
//#define RAYTRACING_AO_HLSL

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"

// Cosine weighted sampling should require ~half the number of rays required by uniformly
// distributed samples to achieve the same variance error.
static const uint SampleCount = 8;

//RaytracingAccelerationStructure Scene : register(t0);
////RWTexture2D<float4> gOutput : register(u0);
////StructuredBuffer<Vertex> Vertices : register(t1);
////ByteAddressBuffer Indices : register(t2);
////StructuredBuffer<VertexPlane> PlaneVertices : register(t3);
////ByteAddressBuffer PlaneIndices : register(t4);
////Texture2D PlaneDiffuseMap : register(t5);
//
//ConstantBuffer<FrameConstants>  frameCB : register(b0);
////ConstantBuffer<HeapIndexConstants> heapIndexCB : register(b1); // type parameter must be a struct
////ConstantBuffer<ObjectConstants> objectCB : register(b1);
//
//SamplerState AnisoWrap : register(s0);
//SamplerState LinearClamp : register(s1);
//SamplerState LinearWrap : register(s2);
//SamplerState PointClamp : register(s3);

//***********************************************************************
//*****------ TraceRay wrappers for radiance and AO rays. -------********
//***********************************************************************

// Trace a radiance ray into the scene.
void TraceRadianceRay(in Ray ray, inout AOPayload payload)
//RtaoPayload TraceRadianceRay(in Ray ray, in Ray ddx, in Ray ddy, in uint currentRayRecursionDepth)
//NormalDepthRayPayload TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth)
//float3 TraceRadianceRay(in Ray ray, in UINT currentRayRecursionDepth)
{
    // We trace only one primary ray per pixel for rtao, so recursion depth does not need to be tracked.

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];
    //RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SrvUAVs::TLASSrv];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    // Set TMin to a zero value to avoid aliasing artifacts along contact areas.
    // Note: make sure to enable face culling so as to avoid surface face fighting.
    // Set primary ray begin & end values equal to near & far plane parameters in projection matrix.
    // See "Essential Ray Generation Shaders", Section 3.2.2 in Ray Tracing Gems II, p.44 for discussion on correct ray clipping.
    rayDesc.TMin = 0;
    rayDesc.TMax = MaxAO_PrimRayLength;

    //NormalDepthRayPayload payload = { float4(0, 0, 0, 0), 0, currentRayRecursionDepth + 1 };
    //NormalDepthRayPayload rayPayload = { float3(0, 0, 0), currentRayRecursionDepth + 1, float3(0, 0, 0), 0.f };
            TraceRay(
        Scene, //g_scene,
        RAY_FLAG_NONE, //RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF, //TraceRayParameters::InstanceMask,
        RayType::Radiance, //TraceRayParameters::HitGroup::Offset[RayType::Radiance],
        0, //TraceRayParameters::HitGroup::GeometryStride,
        RayType::Radiance, //TraceRayParameters::MissShader::Offset[RayType::Radiance],
        rayDesc,
        payload);

    //return payload;
    //return rayPayload.color;
}

// Trace an ambient occlusion ray and return occlusion value if it hits any geometry.
void TraceAORayAndReportIfHit(in Ray ray, inout AOPayload payload)
//uint TraceAORayAndReportIfHit(in Ray ray, inout RtaoPayload payload)
//uint TraceAORayAndReportIfHit(in Ray ray, in uint currentRayRecursionDepth)
//bool TraceAORayAndReportIfHit(in Ray ray, in UINT currentRayRecursionDepth)
{
    payload.occlusion = 1;  // Set to full occlusion, and shadow ray miss shader, if called, will set to zero.

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];
    //RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SrvUAVs::TLASSrv];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    // Set TMin to a zero value to avoid aliasing artifcats along contact areas.
    // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0; //SurfaceEpsilon;
    rayDesc.TMax = OcclusionFadeEnd;

    // Initialize occlusion ray payload.
    // Set the initial value to true since closest and any hit shaders are skipped. 
    // Occlusion miss shader, if called, will set it to 0.
    // Edit: occlusion value is now passed in as a payload parameter to this method.
    //RtaoRayPayload payload = { 1 }; // set to full occlusion & zero depth
    //ShadowRayPayload shadowPayload = { true };
    TraceRay(
        Scene, //g_scene,
        //RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // ~skip closest hit shaders,
        0xFF, //TraceRayParameters::InstanceMask,
        RayType::Occlusion, //TraceRayParameters::HitGroup::Offset[RayType::Shadow],
        0,//MeshGeometries::Count, //TraceRayParameters::HitGroup::GeometryStride,
        //TriangleMeshes::Count, //TraceRayParameters::HitGroup::GeometryStride,
        RayType::Occlusion, //TraceRayParameters::MissShader::Offset[RayType::Shadow],
        rayDesc,
        payload);
        //shadowPayload);

    //return payload.occlusion;// *OcclusionFunction(RayTCurrent()); // Copy occlusion value of the ray payload.
    //return shadowPayload.hit;
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

    AOPayload payload;
    //payload.normalAndDepth = 0; //float4(0, 0, 0, 0);
    //payload.ambientAccess  = 0;
    //payload.occlusion      = 1; // Set to full occlusion.
    //payload.recursionDepth = 0;
    //payload.recursionDepth = currentRayRecursionDepth + 1;
    payload.ddxRay = ddx;
    payload.ddyRay = ddy;

    // Cast a ray into the scene and retrieve a shaded color.
    //UINT currentRecursionDepth = 0;

    TraceRadianceRay(ray, payload);
    //const RtaoPayload payload = TraceRadianceRay(ray, ddx, ddy, 0); // Starting recursion depth is zero.
    //NormalDepthRayPayload payload = TraceRadianceRay(ray, 0);
    //float3 color = TraceRadianceRay(ray, 0);
    //float4 color = TraceRadianceRay(ray, currentRecursionDepth);

    //float3 color = RemoveSRGBCurve(payload.color);
    //float3 finalColor = RemoveSRGBCurve(color.rgb);

    RWTexture2D<float> AmbientOutput = ResourceDescriptorHeap[SrvUAVs::AmbientBuffer0Uav];
    //RWTexture2D<float4> AmbientAccessOutput = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Uav];
    RWTexture2D<float4> NormalDepthOutput  = ResourceDescriptorHeap[SrvUAVs::NormalDepthUav];

    //RWTexture2D<float4> Output = ResourceDescriptorHeap[SrvUAVs::DxrUav];

    // Ambient access buffer stores a scalar value between 0 (full occlusion) and 1 (full visibility).
    const float ambientFactor = 1.f - payload.occlusion;
    // Sharpen the contrast of the rtao map to make the rtao effect more dramatic.
    AmbientOutput[launchIndex] = ambientFactor * ambientFactor;
    //AmbientAccessOutput[launchIndex] = payload.ambientAccess * payload.ambientAccess;
    //AmbientAccessOutput[launchIndex] = saturate(pow(payload.ambientAccess, 6.f));
    //AmbientOcclusionOutput[launchIndex] = float4(color, 1 /*color.a*/ );
    NormalDepthOutput[launchIndex]   = payload.normalAndDepth;
}

[shader("closesthit")]
void ClosestHitCubeShader(inout AOPayload payload, in Attributes attr)
{
    // Direct indexing relies on the correct heap index offset for the geometry that was passed in at
    // shader table creation, and the correct ordering of descriptors in the descriptor heap.
    // Edit: Should no longer be necessary indexing into a structured buffer.

    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()]; // Index into structured buffer with DXR intrinsics.
    //StructuredBuffer<GeometryData> Geometries = ResourceDescriptorHeap[SrvUAVs::GeometryBufferSrv];
    //GeometryData geom = Geometries[InstanceID()]; // Index into structured buffer with DXR intrinsic.

    StructuredBuffer<VertexCube> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    //StructuredBuffer<Vertex> Vertices = ResourceDescriptorHeap[heapIndexCB.vbIndex];
    //ByteAddressBuffer Indices = ResourceDescriptorHeap[heapIndexCB.vbIndex + 1];
    //ByteAddressBuffer Indices = ResourceDescriptorHeap[SrvUAVs::CubeIndexBufferSrv];
    //StructuredBuffer<Vertex> Vertices = ResourceDescriptorHeap[SrvUAVs::CubeVertexBufferSrv];

    //// Get the base index of the triangle's first 16 or 32 bit index.
    //uint isIndex16 = geomData.isIndex16;
    //uint indexSizeInBytes = isIndex16 ? 2 : 4;
    ////uint indexSizeInBytes = 2;
    //uint indicesPerTriangle = 3;
    //uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    //uint baseIndex = PrimitiveIndex() * triangleIndexStride;

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

    // Random number seed generation.
    // Where is this thread's ray on screen?
    const uint2 launchIndex = DispatchRaysIndex().xy;
    const uint2 launchDim   = DispatchRaysDimensions().xy;

    // Initialize a random seed, per-pixel, based on a screen position and temporally varying count
    uint randSeed = InitRand(launchIndex.x + launchIndex.y * launchDim.x, frameCB.frameCount /*, 16 */);

    //Texture2D RandVecMap = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Uav]; // introduced in SM6.6

    // Get two random numbers for uv-coords into the random vector map
    //float2 rand_uv = float2(nextRand(randSeed), nextRand(randSeed));

    // Extract random vector and map from [0,1] --> [-1, +1].
    //float3 randVec = normalize(2.f * RandVecMap.SampleLevel(PointClamp, rand_uv, 0).xyz - 1.f);
    //float3 randVec = normalize(2.f * RandVecMap.SampleLevel(LinearClamp, rand_uv, 0).rgb - 1.f);

    // Ambient occlusion component.
    // Trace ambient occlusion rays about hit position in the hemisphere oriented by the world normal.
    float occlusionSum = 0; // Start value of zero occlusion.

    // Sample neighboring points about p in the hemisphere oriented by n.
    [unroll]
    for (uint i = 0; i < SampleCount; ++i)
    {
        // Get two random numbers for uv-coords into the random vector map
        //float2 rand_uv = float2(nextRand(randSeed), nextRand(randSeed));

        // Extract random vector and map from [0,1] --> [-1, +1].
        //float3 randVec = normalize(2.f * RandVecMap.SampleLevel(PointClamp, rand_uv, 0).xyz - 1.f);
        //float3 randVec = normalize(2.f * RandVecMap.SampleLevel(LinearClamp, rand_uv, 0).rgb - 1.f);
        
        // Our offset vectors are fixed and uniformly distributed (so that our offset vectors
        // do not clump in the same direction).  If we reflect them about a random vector
        // then we get a random uniform distribution of offset vectors.
        //float3 offset = reflect(frameCB.offsetVectors[i].xyz, randVec);

        // Sample cosine-weighted hemisphere around surface normal to pick a random ray direction
        const float3 worldDir = GetCosHemisphereSample(randSeed, worldNormal);

        // Flip offset vector if it is behind the plane defined by (p, n).
        //float flip = sign(dot(offset, normalW));
        //float dp = dot(flip * offset, normalW);
        const float dp = saturate(dot(worldDir, worldNormal));

        // Calculate an occlusion ray origin offset to avoid self-intersection.
        const float3 offsetOrigin = GetOffsetRayOrigin(worldPos, worldNormal);
        const Ray aoRay = { offsetOrigin, worldDir };
        //const Ray aoRay = { worldPos, worldDir};
        //Ray shadowRay = { hitPosition, offset };
        //Ray aoRay = { hitPosition, flip * offset };

        TraceAORayAndReportIfHit(aoRay, payload);

        // Get sample using Monte Carlo estimation.
        // See Pharr, "On the Importance of Sampling", Ray Tracing Gems, p.211, Eq.7
        const float occlusion = dp * payload.occlusion;
        //const float occlusion = dp * (float)TraceAORayAndReportIfHit(aoRay, payload.recursionDepth);
        //float occlusion = TraceAORayAndReportIfHit(aoRay, rayPayload.recursionDepth);

        occlusionSum += occlusion;
        //occlusionSum += TraceAORayAndReportIfHit(aoRay, rayPayload.recursionDepth);
        //bool shadowRayHit = TraceAORayAndReportIfHit(aoRay, rayPayload.recursionDepth);

        //if (shadowRayHit)
        //	occlusionSum -= 0.1f;
    }

    // Directional light shading.
    //float NdotL = dot(-frameCB.lightDir, normalW); // negate light direction
    //float3 diffuseColor = shadowRayHit ? float3(0, 0, 0) : frameCB.lightDiffuse.rgb * saturate(NdotL);
    //float3 hitColor = objectCB.albedo.rgb * (diffuseColor + frameCB.lightAmbient.rgb);
    
    // Get average occlusion value and set it into the payload.
    occlusionSum /= (float)SampleCount;
    payload.occlusion = occlusionSum;
    //float occlusionVal = saturate(occlusionSum);

    //float access = 1.f - occlusionSum;
    //access = saturate(pow(access, 6.f)); // sharpen the contrast

    //payload.ambientAccess = saturate(1.f - occlusionSum);
    //payload.ambientAccess = 1.f - occlusionSum;
    //rayPayload.color = float3(access, access, access);
    //rayPayload.color = float4(occlusionVal, occlusionVal, occlusionVal, 1);
    //rayPayload.color = float4(hitColor, objectCB.albedo.a);

    payload.normalAndDepth = float4(worldNormal, RayTCurrent());
}

[shader("closesthit")]
void ClosestHitShader(inout AOPayload payload, Attributes attr)
{
    // Direct indexing relies on the correct heap index offset for the geometry that was passed in at
    // shader table creation, and the correct ordering of descriptors in the descriptor heap.
    // Edit: Should no longer be necessary indexing into a structured buffer.

    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()+GeometryIndex()]; // Index into structured buffer with DXR intrinsics.
    //StructuredBuffer<GeometryData> Geometries = ResourceDescriptorHeap[SrvUAVs::GeometryBufferSrv];
    //GeometryData geom = Geometries[InstanceID()]; // Index into structured buffer with DXR intrinsic.

    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Get the base index of the triangle's first 16 or 32 bit index.
    //uint isIndex16 = geomData.isIndex16;
    //uint indexSizeInBytes = isIndex16 ? 2 : 4;
    ////uint indexSizeInBytes = 2;
    //uint indicesPerTriangle = 3;
    //uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    //uint baseIndex = PrimitiveIndex() * triangleIndexStride;

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

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.
    const float3 worldPos    = HitWorldPosition();
    const float3 worldNormal = HitAttribute(vertexNormals, attr);    // Get interpolated normal at the hit position.
    //float3 hitNormal = HitAttribute(vertexNormals, attr);    // Get interpolated normal at the hit position.
    //hitNormal = mul(hitNormal, (float3x3)ObjectToWorld4x3());

    // Random number seed generation.
    // Where is this thread's ray on screen?
    const uint2 launchIndex = DispatchRaysIndex().xy;
    const uint2 launchDim   = DispatchRaysDimensions().xy;

    // Initialize a random seed, per-pixel, based on a screen position and temporally varying count.
    uint randSeed = InitRand(launchIndex.x + launchIndex.y * launchDim.x, frameCB.frameCount /* , 16 */);

    // Ambient occlusion component.
    // Trace ambient occlusion rays about hit position in the hemisphere oriented by the world normal.
    float occlusionSum = 0; // Start value of zero occlusion.

    // Sample neighboring points about p in the hemisphere oriented by n.
    [unroll]
    for (uint i = 0; i < SampleCount; ++i)
    {
        // Sample cosine-weighted hemisphere around surface normal to pick a random ray direction.
        const float3 worldDir = GetCosHemisphereSample(randSeed, worldNormal);
        const float dp = saturate(dot(worldDir, worldNormal));

        // Calculate an occlusion ray origin offset to avoid self-intersection.
        const float3 offsetOrigin = GetOffsetRayOrigin(worldPos, worldNormal);
        const Ray aoRay = { offsetOrigin, worldDir };
        //const Ray aoRay = { worldPos, worldDir };

        TraceAORayAndReportIfHit(aoRay, payload);

        // Get sample using Monte Carlo estimation.
        // See Pharr, "On the Importance of Sampling", Ray Tracing Gems, p.211, Eq.7
        const float occlusion = dp * payload.occlusion;
        //const float occlusion = dp * (float)TraceAORayAndReportIfHit(aoRay, payload.recursionDepth);

        occlusionSum += occlusion;
    }

    // Get average occlusion value and set it into the payload.
    occlusionSum /= (float)SampleCount;
    payload.occlusion = occlusionSum;

    //payload.ambientAccess = saturate(1.f - occlusionSum);
    //payload.ambientAccess = 1.f - occlusionSum;
    payload.normalAndDepth = float4(worldNormal, RayTCurrent());
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
void AnyHitShader(inout AOPayload payload, Attributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()+GeometryIndex()]; // Index into structured buffer with DXR intrinsic.

    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Get the base index of the triangle's first 16 or 32 bit index.
    //uint isIndex16 = objData.geometryData.isIndex16;
    //uint indexSizeInBytes = isIndex16 ? 2 : 4;
    //uint indicesPerTriangle = 3;
    //uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    //uint baseIndex = PrimitiveIndex() * triangleIndexStride;

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
void MissShader_CameraRay(inout AOPayload payload)
{
    payload.normalAndDepth = 0;
    payload.occlusion = 0;
    //payload.ambientAccess = 1;
    //payload.color = float3(1, 1, 1);
}

[shader("miss")]
void MissShader_Occlusion(inout AOPayload payload)
//void MyMissShader_AORay(inout ShadowRayPayload payload)
{
    payload.occlusion = 0; // i.e. no occlusion if ray misses all geometry
    //payload.hit = false;
}
//#endif // RAYTRACING_AO_HLSL
