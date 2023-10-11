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

//#ifndef RAYTRACING_HLSL
//#define RAYTRACING_HLSL

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"
#include "CommonPBR.hlsli"

//RaytracingAccelerationStructure Scene : register(t0);
////RWTexture2D<float4> gOutput : register(u0);
////StructuredBuffer<Vertex> Vertices : register(t1);
////ByteAddressBuffer Indices : register(t2);
////StructuredBuffer<VertexPlane> PlaneVertices : register(t3);
////ByteAddressBuffer PlaneIndices : register(t4);
////Texture2D PlaneDiffuseMap : register(t5);
//
//ConstantBuffer<FrameConstants>  frameCB : register(b0);
////ConstantBuffer<ObjectConstants> objectCB : register(b1);
//
//SamplerState AnisoWrap : register(s0);
//SamplerState LinearClamp : register(s1);

// declare global variables as static in HLSL
//static uint currentRecursionDepth;

//***************************************************************************
//*****------ TraceRay wrappers for radiance and shadow rays. -------********
//***************************************************************************

// Trace a radiance ray into the scene and returns a shaded color.
void TraceRadianceRay(in Ray ray, inout ColorPayload payload)
//float3 TraceRadianceRay(in Ray ray, in Ray ddx, in Ray ddy, in uint currentRayRecursionDepth)
//float3 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth)
{
    if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        payload.color = 0; //float3(0, 0, 0);
        return;
        //return 0;
        //float3(0, 0, 0);
    }

    payload.recursionDepth++;

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];
    //RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SrvUAVs::TLASSrv];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    // Set TMin to a nonzero value to avoid aliasing artifacts along contact areas.
    // Note: make sure to enable face culling so as to avoid surface face fighting.
    // Set primary ray begin & end values equal to near & far plane parameters in projection matrix.
    // See "Essential Ray Generation Shaders", Section 3.2.2 in Ray Tracing Gems II, p.44 for discussion on correct ray clipping.
    rayDesc.TMin = 0; //SurfaceEpsilon;
    rayDesc.TMax = MaxPrimaryRayLength;

    //ColorRayPayload rayPayload = { float3(0, 0, 0), currentRayRecursionDepth + 1 };
    TraceRay(Scene, //g_scene,
        RAY_FLAG_NONE, //RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF, //TraceRayParameters::InstanceMask,
        RayType::Radiance, //TraceRayParameters::HitGroup::Offset[RayType::Radiance],
        0, //TraceRayParameters::HitGroup::GeometryStride,
        RayType::Radiance, //TraceRayParameters::MissShader::Offset[RayType::Radiance],
        rayDesc,
        payload);

    //return payload.color;
}

/*
// Trace a shadow ray and return true if it hits any geometry.
void TraceShadowRayAndReportIfHit(in Ray ray, inout ColorPayload payload)
//bool TraceShadowRayAndReportIfHit(in Ray ray, inout ColorPayload payload)
//bool TraceShadowRayAndReportIfHit(in Ray ray, in Ray ddx, in Ray ddy, in uint currentRayRecursionDepth)
//bool TraceShadowRayAndReportIfHit(in Ray ray, in uint currentRayRecursionDepth)
{
    if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        payload.isShadowRayMiss = true;
        //payload.isShadowRayHit = false;
        return;
        //return false;
    }

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];
    //RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SrvUAVs::TLASSrv];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin    = ray.origin;
    rayDesc.Direction = ray.direction;
    // Set TMin to a nonzero value to avoid aliasing artifcats along contact areas.
    // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0; //SurfaceEpsilon;
    //rayDesc.TMin = 0.001f;
    rayDesc.TMax = MaxRayLength;

    // Initialize shadow ray payload.
    // Set the initial shadow miss flag value to false since closest and any hit shaders are skipped. 
    // Shadow miss shader, if called, will set shadow miss flag to true.
    // Edit: shadow hit flag is now passed in as a payload parameter to this method.
    // ShadowRayPayload payload = { true, (AuxilliaryRay)ddx, (AuxilliaryRay)ddy };
    //ShadowRayPayload shadowPayload = { true, ddx.origin, ddx.direction, ddy.origin, ddy.direction };
    //ShadowRayPayload shadowPayload = { true };
    TraceRay(
        Scene, //g_scene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        //| RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        //| RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // ~skip closest hit shaders,
        0xFF, //TraceRayParameters::InstanceMask,
        RayType::Occlusion, //TraceRayParameters::HitGroup::Offset[RayType::Shadow],
        0,//MeshGeometries::Count, //TraceRayParameters::HitGroup::GeometryStride,
        //TriangleMeshes::Count, //TraceRayParameters::HitGroup::GeometryStride,
        RayType::Occlusion, //TraceRayParameters::MissShader::Offset[RayType::Shadow],
        rayDesc,
        payload);

    //return payload.isShadowRayHit;
}
*/

[shader("raygeneration")]
void RaygenShader()
{
    // Get the location within the dispatched 2D grid of work items
    // (often maps to pixels, so this could represent a pixel coordinate).
    const uint2 launchIndex    = DispatchRaysIndex().xy; // Get screen pixel coords.
    const float3 cameraPos     = frameCB.cameraPos.xyz;
    const float4x4 invViewProj = frameCB.invViewProj;
    //float2 dims = DispatchRaysDimensions().xy; // get screen pixel dimensions
    //float2 d = (launchIndex.xy + 0.5f) / dims.xy * 2.f - 1.f; // calculate screen space coords

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    //GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);
    const Ray ray = GenerateCameraRay(launchIndex, cameraPos, invViewProj);
    const Ray ddx = GenerateCameraRay(uint2(launchIndex.x + 1, launchIndex.y), cameraPos, invViewProj);
    const Ray ddy = GenerateCameraRay(uint2(launchIndex.x, launchIndex.y + 1), cameraPos, invViewProj);

    ColorPayload payload;
    payload.color          = 0; //float3(0, 0, 0);
    payload.recursionDepth = 0; // Starting recursion depth is zero.
    //payload.isShadowRayMiss= false;  // Set to false, and shadow ray miss shader, if called, will set to true.
    //payload.isShadowRayHit = true;
    payload.ddxRay         = ddx;
    payload.ddyRay         = ddy;

    // Cast a ray into the scene and retrieve a shaded color.
    //UINT currentRecursionDepth = 0;
    TraceRadianceRay(ray, payload);
    //const float3 color = TraceRadianceRay(ray, ddx, ddy, 0); // Starting recursion depth is zero.
    //float3 color = TraceRadianceRay(ray, 0); // starting recursion depth is zero
    //float4 color = TraceRadianceRay(ray, currentRecursionDepth);

    /*
    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    //ray.Origin = float3(d.x, -d.y, -1);	// +y points down in screen space
    //ray.Direction = float3(0, 0, 1);	// +z points into screen in post-projection space
    rayDesc.TMin = 0;// 0.001;
    rayDesc.TMax = 10000;

    // Initialize the ray payload
    RayPayload payload = { float4(0, 0, 0, 0), currentRecursionDepth + 1 };
    //payload.colorAndDistance = float4(0.9, 0.6, 0.2, 1);
    //payload.colorAndDistance = float4(0, 0, 0, 0);

    // Trace the ray
    TraceRay(
        // Parameter name: AccelerationStructure
        Scene,
        // Parameter name: RayFlags
        // Flags can be used to specify the behavior upon hitting a surface.
        // We are using a RH coordinate system, which reverses the triangle facing direction expected by DXR.
        // NB: We are overriding this flag in the application.
        RAY_FLAG_NONE,// RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        // Parameter name: InstanceInclusionMask
        // Instance inclusion mask, which can be used to mask out some geometry to this ray by
        // and-ing the mask with a geometry mask. The 0xFF flag then indicates no geometry will be masked.
        0xFF,//~0,
        // Parameter name: RayContributionToHitGroupIndex
        // Depending on the type of ray, a given object can have several hit groups attached
        // (ie. what to do when hitting to compute regular shading, and what to do when hitting
        // to compute shadows). Those hit groups are specified sequentially in the SBT, so the value
        // below indicates which offset (on 4 bits) to apply to the hit groups for this ray. In this
        // sample we only have one hit group per object, hence an offset of 0.
        0,
        // Parameter name: MultiplierForGeometryContributionToHitGroupIndex
        // The offsets in the SBT can be computed from the object ID, its instance ID, but also simply
        // by the order the objects have been pushed in the acceleration structure. This allows the
        // application to group shaders in the SBT in the same order as they are added in the AS, in
        // which case the value below represents the stride (4 bits representing the number of hit
        // groups) between two consecutive objects.
        0,// 1,
        // // Parameter name: MissShaderIndex
        // Index of the miss shader to use in case several consecutive miss shaders are present in the
        // SBT. This allows to change the behavior of the program when no geometry have been hit, for
        // example one to return a sky color for regular rendering, and another returning a full
        // visibility value for shadow rays. This sample has only one miss shader, hence an index 0.
        0,
        // Parameter name: Ray
        // Ray information to trace.
        rayDesc,
        // Parameter name: Payload
        // Payload associated to the ray, which will be used to communicate between the hit/miss
        // shaders and the raygen.
        payload);
    */

    //color = RemoveSRGBCurve(color);

    // We can view other textures for debugging purposes.
    RWTexture2D<float4> Output = ResourceDescriptorHeap[SrvUAVs::DxrUav];
    //Texture2D<float> AmbientMap = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Srv];
    //Texture2D<float> ShadowMap = ResourceDescriptorHeap[SrvUAVs::ShadowBuffer0Srv];
    //Texture2D<float>	AmbientMap = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Uav];
    //Texture2D<float4>	NormalDepthMap = ResourceDescriptorHeap[SrvUAVs::NormalDepthUav];

    // The ambient access resource is one-quarter the size of the render target, so use bilinear sampling.
    /*
    uint width, height;
    Output.GetDimensions(width, height);

    float u = (float)launchIndex.x / (float)width;
    float v = (float)launchIndex.y / (float)height;

    float2 texCoords = float2(u, v);
    //*/
    
    //float2 texCoords	 = GetTextureCoords();

    //float  ambientAccess = AmbientMap.SampleLevel(LinearClamp, texCoords, 0);
    //payload.color = ambientAccess.rrr;

    //float  shadowShade = ShadowMap.SampleLevel(LinearClamp, texCoords, 0);
    //payload.color = shadowShade.rrr;

    Output[launchIndex] = float4(payload.color, 1 /*color.a*/);

    //Output[launchIndex] = float4(texCoords.x, texCoords.y, 0, 1);
    //float ambientAccess = AmbientMap[launchIndex];
    
    //float4 normalAndDepth = NormalDepthMap.SampleLevel(LinearClamp, texCoords, 0);
    
    // We are mapping visualised normals to interval [0,1].
    //Output[launchIndex] = /*normalAndDepth;*/ float4(normalAndDepth.xyz * 0.5f + 0.5f, normalAndDepth.z);
    //Output[launchIndex] = NormalDepthMap[launchIndex];
    //gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}

[shader("closesthit")]
void ClosestHitCubeShader(inout ColorPayload payload, Attributes attr)
//void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()]; // Index into structured buffer with DXR intrinsics.

    StructuredBuffer<PrevFrameData> PrevFrameBuffer = ResourceDescriptorHeap[frameCB.prevFrameBufferSrvID];
    PrevFrameData prevFrameData = PrevFrameBuffer[InstanceID()];

    Texture2D<float3> AlbedoMap    = ResourceDescriptorHeap[inst.material.albedoTexIndex];
    //Texture2D NormalMap   = ResourceDescriptorHeap[objData.material.normalTexIndex];
    Texture2D<float3> EmissiveMap  = ResourceDescriptorHeap[inst.material.emissiveTexIndex];
    Texture2D<float4> RMAMap       = ResourceDescriptorHeap[inst.material.RMATexIndex];
    //TextureCube<float3> DiffIBLMap = ResourceDescriptorHeap[objData.material.diffuseIBLTexIndex];
    //TextureCube<float3> SpecIBLMap = ResourceDescriptorHeap[objData.material.specularIBLTexIndex];
    Texture2D<float>  AmbientBufferCurr = ResourceDescriptorHeap[SrvUAVs::AmbientBuffer0Srv];
    Texture2D<float>  ShadowBufferCurr  = ResourceDescriptorHeap[SrvUAVs::ShadowBuffer0Srv];
    Texture2D<float3> ColorBufferPrev   = ResourceDescriptorHeap[SrvUAVs::DxrPrevSrv];
    //Texture2D<float>  AmbientBufferPrev = ResourceDescriptorHeap[SrvUAVs::AmbientBufferPrevSrv];
    //Texture2D<float>  ShadowBufferPrev  = ResourceDescriptorHeap[SrvUAVs::ShadowBufferPrevSrv];

    StructuredBuffer<VertexCube> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    //StructuredBuffer<Vertex> Vertices = ResourceDescriptorHeap[objectCB.vbIndex];
    //ByteAddressBuffer Indices = ResourceDescriptorHeap[objectCB.vbIndex + 1];
    //ByteAddressBuffer Indices = ResourceDescriptorHeap[SrvUAVs::CubeIndexBufferSrv];
    //StructuredBuffer<Vertex> Vertices = ResourceDescriptorHeap[SrvUAVs::CubeVertexBufferSrv];

    // Get the base index of the triangle's first 16 or 32 bit index.
    //uint isIndex16 = objData.geometryData.isIndex16;
    //uint indexSizeInBytes = isIndex16 ? 2 : 4;
    ////uint indexSizeInBytes = 2;
    //uint indicesPerTriangle = 3;
    //uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    //uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, PrimitiveIndex(), Indices);
    //const uint3 indices = isIndex16 ? Load3x16BitIndices(baseIndex, Indices) : Indices.Load3(baseIndex);
    //const uint3 indices = Load3x16BitIndices(baseIndex, Indices);

    //float4 firstVertexColor = float4(0, 0, 0, 0);
    //
    //// Use DXR provided InstanceID() to determine color for first vertex.
    //switch (InstanceID())
    //{
    //case 0:
    //	firstVertexColor = float4(1, 0, 0, 1); // red
    //	break;
    //case 1:
    //	firstVertexColor = float4(0, 1, 0, 1); // green
    //	break;
    //case 2:
    //	firstVertexColor = float4(0, 0, 1, 1); // blue
    //	break;
    //}

    // Retrieve corresponding vertex positions for the triangle vertices.
    // Texture filtering with ray differentials was solved by transforming vertex positions to world space.
    const float3 vertexPositions[3] =
    {
        mul(float4(Vertices[indices[0]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[1]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[2]].position, 1), ObjectToWorld4x3()),
    };
    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] = {
        //firstVertexColor,
        mul(Vertices[indices[0]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[1]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[2]].normal, (float3x3)ObjectToWorld4x3()),
    };
    // Retrieve corresponding texcoords for the triangle vertices.
    const float2 vertexTexCoords[3] = {
        Vertices[indices[0]].texCoord,
        Vertices[indices[1]].texCoord,
        Vertices[indices[2]].texCoord,
    };

    /*
    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = shadowRay.origin;
    rayDesc.Direction = shadowRay.direction;
    // Set TMin to a zero value to avoid aliasing artifacts along contact areas.
    // Note: make sure to enable back-face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0;
    rayDesc.TMax = 10000;

    // Initialize shadow ray payload.
    // Set the initial value to true since closest and any hit shaders are skipped. 
    // Shadow miss shader, if called, will set it to false.
    ShadowRayPayload shadowPayload = { true };
    if (payload.recursionDepth >= MAX_RAY_RECURSION_DEPTH)
        shadowPayload.hit = false;
    else
        TraceRay(Scene,
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES
            | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
            | RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
            | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // ~skip closest hit shaders,
            0xFF,//TraceRayParameters::InstanceMask,
            1,//TraceRayParameters::HitGroup::Offset[RayType::Shadow],
            2,// InstanceID(),//TraceRayParameters::HitGroup::GeometryStride,
            1,//TraceRayParameters::MissShader::Offset[RayType::Shadow],
            rayDesc,
            shadowPayload);

    //bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, rayPayload.recursionDepth);
    bool shadowRayHit = shadowPayload.hit;
    */

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.
    const float3 worldPos     = HitWorldPosition();
    const float3 worldNormal  = vertexNormals[0]; // Vertex normals per triangle are identical, so no interpolation required.
    //const float3 worldNormal  = HitAttribute(vertexNormals, attr);
    const float2 uv           = HitAttribute(vertexTexCoords, attr);
    //int2 uvScaled = floor(texCoord * 1024.f);
    const float3 eyePos       = frameCB.cameraPos.xyz;
    float3 lightAmbient       = frameCB.lightAmbient.rgb; // Requires scaling by the ambient buffer.
    float3 lightDiffuse       = frameCB.lightDiffuse.rgb; // Requires attenuation by the bump shadowing factor.

    // Before lighting, peturb the surface's normal by the one given in normal map.
    //float3 localNormal = TwoChannelNormalX2(NormalTexture.Sample(SurfaceSampler, pin.TexCoord).xy);

    // Sample the material texture.
    //---------------------------------------------------------------------------------------------
    // Compute partial derivatives of UV coordinates:
    //
    //  1) Construct a plane from the hit triangle
    //  2) Intersect two helper rays with the plane:  one to the right and one down
    //  3) Compute barycentric coordinates of the two hit points
    //  4) Reconstruct the UV coordinates at the hit points
    //  5) Take the difference in UV coordinates as the partial derivatives X and Y

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

    float3 albedo         = AlbedoMap.SampleGrad(AnisoClamp, uv, pd.ddxUV, pd.ddyUV);
    //float3 localNormal = NormalMap.SampleLevel(AnisoClamp, uv, 0).xyz;
    const float3 emissive = EmissiveMap.SampleGrad(AnisoClamp, uv, pd.ddxUV, pd.ddyUV);
    const float4 rma      = RMAMap.SampleGrad(AnisoClamp, uv, pd.ddxUV, pd.ddyUV);
    //float3 albedo   = AlbedoMap.SampleLevel(AnisoClamp, uv, 0);
    ////float3 localNormal = NormalMap.SampleLevel(AnisoClamp, uv, 0).xyz;
    //float3 emissive = EmissiveMap.SampleLevel(AnisoClamp, uv, 0);
    //float4 rma      = RMAMap.SampleLevel(AnisoClamp, uv, 0);

    albedo = RemoveSRGBCurve(albedo) * inst.material.albedo.rgb; // Modulate albedo by instance's color and convert to linear color space.
    //albedo   = RemoveSRGBCurve(albedo) * objData.material.albedo.rgb; // Also modulate by instance's albedo constant.
    //emissive = RemoveSRGBCurve(emissive);
    //rma.rgb	 = RemoveSRGBCurve(rma.rgb);

    const float3 V = normalize(eyePos - worldPos);  // View vector.
    const float3 L = -frameCB.lightDir;				// Light vector ("to light" opposite of light's direction).
    const float3 N = worldNormal;// We need to manually calculate derivatives to use the function below, so just use world normal for now.
    //float3 N = PeturbNormal(localNormal, hitPosition, normalW, uv);

    /*
    // Trace a shadow ray.
    // Perform before tracing a reflection ray, because reflection rays increase the ray bounce count.
    // Calculate a ray origin offset to avoid self-intersection.
    const float3 offsetOrigin = GetOffsetRayOrigin(worldPos, worldNormal);
    const Ray shadowRay    = { offsetOrigin, L };
    //const Ray shadowRay = { worldPos, L };
    const Ray ddxShadowRay = { xOffsetPoint, L };
    const Ray ddyShadowRay = { yOffsetPoint, L };

    // Update payload with auxilliary shadow rays.
    payload.ddxRay = (AuxilliaryRay)ddxShadowRay;
    payload.ddyRay = (AuxilliaryRay)ddyShadowRay;

    TraceShadowRayAndReportIfHit(shadowRay, payload);

    const uint shadowMask = (uint)payload.isShadowRayMiss;
    //const bool isShadowRayHit = payload.isShadowRayHit;
    //const bool isShadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, payload);
    //const bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, ddxShadowRay, ddyShadowRay, payload.recursionDepth);
    //bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, rayPayload.recursionDepth);
    */

    // Trace a reflection ray.
    const float3 R = reflect(WorldRayDirection(), N);
    const float3 offsetOrigin = GetOffsetRayOrigin(worldPos, worldNormal);

    // Use the ray origin offset calculated above to avoid self-intersection.
    const Ray reflectionRay    = { offsetOrigin, R };
    //const Ray reflectionRay    = { worldPos, R };
    const Ray ddxReflectionRay = { xOffsetPoint, R };
    const Ray ddyReflectionRay = { yOffsetPoint, R };
    //Ray reflectionRay = { hitPosition, reflect(WorldRayDirection(), N) };

    // Update payload auxilliary rays.
    payload.ddxRay = ddxReflectionRay;
    payload.ddyRay = ddyReflectionRay;

    TraceRadianceRay(reflectionRay, payload);

    const float3 reflectionColor = payload.color;
    //const float3 reflectionColor = TraceRadianceRay(reflectionRay, ddxReflectionRay, ddyReflectionRay, payload.recursionDepth);
    //float3 reflectionColor = TraceRadianceRay(reflectionRay, rayPayload.recursionDepth);
    //float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), normalW, objectCB.material.albedo);
    //reflectedColor = objectCB.material.reflectanceCoef * fresnelR * reflectionColor;

    // The occlusion buffers are quarter resolution, so we use bilinear sampling.
    const float2   uvCurr    = GetTextureCoords();
    const float4x4 prevWorld = prevFrameData.world;
    const float4x4 prevViewProjT = frameCB.prevViewProjTex;
    const float2   uvPrev  = CalculatePrevFrameTexCoords(worldPos, prevWorld, prevViewProjT);

    const float  ambientCurr = AmbientBufferCurr.SampleLevel(LinearClamp, uvCurr, 0);
    //const float  ambientPrev = AmbientBufferPrev.SampleLevel(LinearClamp, uvPrev, 0);
    // We also perform cheap temporal filtering by averaging with the previous frame value.
    const float  ambientFactor = ambientCurr; // lerp(ambientCurr, ambientPrev, 0.5f);

    lightAmbient *= ambientFactor;  // Scale the ambient lighting term by the ambient factor.

    // Sample the shadow screen mask.
    const float shadowCurr = ShadowBufferCurr.SampleLevel(LinearClamp, uvCurr, 0);
    //const float shadowPrev = ShadowBufferPrev.SampleLevel(LinearClamp, uvPrev, 0);
    // We also perform cheap temporal filtering by averaging with the previous frame value.
    const float shadowFactor = shadowCurr; // lerp(shadowCurr, shadowPrev, 0.5f);

    // Attenuate diffuse light intensity by bump shadowing factor.
    lightDiffuse *= BumpShadowingFactor(worldNormal, N, L);

    // Calculate physically based lighting.
    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel.
    float3 color = LightSurface(V, N, L, 1, lightDiffuse, albedo, rma.g, rma.b, rma.r, rma.a, lightAmbient,
        reflectionColor, shadowFactor); //isShadowRayHit);
        //, DiffIBLMap, SpecIBLMap);

    color += emissive;    // Add emissive lighting constribution.

    // Sample previous frame's output and blend with current frame color, except if this is the first rendered frame.
    if (frameCB.isFirstFrame)
    {
        payload.color = color;
        return;
    }

    const float3 prevColor = ColorBufferPrev.SampleLevel(LinearClamp, uvPrev, 0);
    color = lerp(color, prevColor, 0.5f);

    // Calculate final color.
    //float3 phongColor = CalculatePhongLighting(objData.material.diffuseColor, normalW, shadowRayHit,
    //	frameCB.lightDir.rgb, frameCB.lightDiffuse.rgb, ambientLight,//frameCB.lightAmbient.rgb,
    //	objData.material.diffuseCoef, objData.material.specularCoef, objData.material.specularPower);

    //float3 phongColor = CalculatePhongLighting(objectCB.material.albedo, normalW, shadowRayHit,
    //	frameCB.lightDir.rgb, frameCB.lightDiffuse.rgb, ambientLight,//frameCB.lightAmbient.rgb,
    //	1.f, 0.4f, 50.f); // hardcoded material specular coefficients for cubes
    //	//objectCB.diffuseCoef, objectCB.specularCoef, objectCB.specularPower);

    // This lighting computation has been replaced by above phong lighting function.
    //float NdotL = dot(-frameCB.lightDir, normalW); // negate light direction
    //float3 diffuseColor = shadowRayHit ? float3(0,0,0) : frameCB.lightDiffuse.rgb * saturate(NdotL);
    //float3 hitColor = objectCB.albedo.rgb * (diffuseColor + frameCB.lightAmbient.rgb);  // tints ambient light with object diffuse color

    //float3 diffuseColor = frameCB.lightDiffuse.rgb * saturate(NdotL);// *objectCB.diffuse.rgb;
    //float3 hitColor = objectCB.diffuse.rgb * (diffuseColor + frameCB.lightAmbient.rgb);  // tints ambient light with object diffuse color

    //float3 hitColor = mul(vertexNormals[0],(float3x3)ObjectToWorld4x3()); // must use ObjectToWorld4x3() intrinsic instead of ObjectToWorld3x4()
    //float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    //float3 hitColor = vertexColors[0].rgb * barycentrics.x + vertexColors[1].rgb * barycentrics.y + vertexColors[2].rgb * barycentrics.z;

    //hitColor = hitColor * materialCB.diffuse.rgb;

    // Alternative calculation for hit position color (same result).
    //float3 hitColor = vertexColors[0].rgb +
    //	attr.barycentrics.x * (vertexColors[1].rgb - vertexColors[0].rgb) +
    //	attr.barycentrics.y * (vertexColors[2].rgb - vertexColors[0].rgb);

    //rayPayload.color = color * sceneOcclusion; // apply scene RTAO.
    payload.color = color;
    //rayPayload.color = phongColor; // we're not applying reflected and texture color here.
    //rayPayload.color = hitColor;// float4(hitColor, objectCB.albedo.a);

    //rayPayload.color = float4(hitColor, objectCB.diffuse.a);
    //payload.colorAndDistance = float4(hitColor, 1);
    //payload.colorAndDistance = float4(barycentrics, 1);
    //payload.colorAndDistance = float4(1, 1, 0, RayTCurrent());
}

[shader("closesthit")]
void ClosestHitShader(inout ColorPayload payload, Attributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()+GeometryIndex()]; // Index into structured buffer with DXR intrinsics.

    StructuredBuffer<PrevFrameData> PrevFrameBuffer = ResourceDescriptorHeap[frameCB.prevFrameBufferSrvID];
    PrevFrameData prevFrameData = PrevFrameBuffer[InstanceIndex()]; // Note we can't use user defined InstanceID() now.

    Texture2D<float3> AlbedoMap = ResourceDescriptorHeap[inst.material.albedoTexIndex];
    Texture2D<float2> NormalMap = ResourceDescriptorHeap[inst.material.normalTexIndex]; // In preparation for sampling a BC5_SNORM texture.
    //Texture2D<float3> NormalMap = ResourceDescriptorHeap[inst.material.normalTexIndex];
    Texture2D<float4> RMAMap    = ResourceDescriptorHeap[inst.material.RMATexIndex]; // Alpha channel contains specular map.
    //TextureCube<float3> DiffIBLMap = ResourceDescriptorHeap[objData.material.diffuseIBLTexIndex];
    //TextureCube<float3> SpecIBLMap = ResourceDescriptorHeap[objData.material.specularIBLTexIndex];
    //Texture2D DiffuseMap = ResourceDescriptorHeap[objData.diffuseTextureIndex];
    //Texture2D NormalMap = ResourceDescriptorHeap[objData.normalTextureIndex];
    Texture2D<float>  AmbientBufferCurr = ResourceDescriptorHeap[SrvUAVs::AmbientBuffer0Srv];
    Texture2D<float>  ShadowBufferCurr  = ResourceDescriptorHeap[SrvUAVs::ShadowBuffer0Srv];
    Texture2D<float3> ColorBufferPrev   = ResourceDescriptorHeap[SrvUAVs::DxrPrevSrv];
    //Texture2D<float>  AmbientBufferPrev = ResourceDescriptorHeap[SrvUAVs::AmbientBufferPrevSrv];
    //Texture2D<float>  ShadowBufferPrev  = ResourceDescriptorHeap[SrvUAVs::ShadowBufferPrevSrv];

    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Get the base index of the triangle's first 16 or 32 bit index.
    //bool isIndex16 = objData.geometryData.isIndex16;
    //uint indexSizeInBytes = isIndex16 ? 2 : 4;
    ////uint indexSizeInBytes = 2;
    //uint indicesPerTriangle = 3;
    //uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    //uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, PrimitiveIndex(), Indices);
    //const uint3 indices = isIndex16 ? Load3x16BitIndices(baseIndex, Indices) : Indices.Load3(baseIndex);
    //const uint3 indices = Load3x16BitIndices(baseIndex, Indices);

    // Retrieve corresponding vertex positions for the triangle vertices.
    // Texture filtering with ray differentials was solved by transforming vertex positions to world space.
    const float3 vertexPositions[3] = {
        mul(float4(Vertices[indices[0]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[1]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[2]].position, 1), ObjectToWorld4x3()),
    };
    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] = {
        //firstVertexColor,
        mul(Vertices[indices[0]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[1]].normal, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[2]].normal, (float3x3)ObjectToWorld4x3()),
    };
    // Retrieve corresponding texcoords for the triangle vertices.
    const float  wrapFactor = inst.material.textureWrapFactor;
    const float2 vertexTexCoords[3] = {
        Vertices[indices[0]].texCoord * wrapFactor,
        Vertices[indices[1]].texCoord * wrapFactor,
        Vertices[indices[2]].texCoord * wrapFactor,
    };
    // Retrieve corresponding vertex tangents for the triangle vertices.
    const float3 vertexTangents[3] = {
        //firstVertexColor,
        mul(Vertices[indices[0]].tangent, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[1]].tangent, (float3x3)ObjectToWorld4x3()),
        mul(Vertices[indices[2]].tangent, (float3x3)ObjectToWorld4x3()),
    };

    // Compute the triangle's normal.
    // This is redundant and done for illustration purposes 
    // as all the per-vertex normals are the same and match triangle's normal in this sample. 
    //float3 triangleNormal = HitAttribute(vertexNormals, attr);

    // PERFORMANCE TIP: it is recommended to avoid values carry over across TraceRay() calls. 
    // Therefore, in cases like retrieving HitWorldPosition(), it is recomputed every time.
    const float3 worldPos     = HitWorldPosition();
    const float3 worldNormal  = HitAttribute(vertexNormals, attr);
    const float2 uv           = HitAttribute(vertexTexCoords, attr);
    //int2 uvScaled = floor(texCoord * 1024.f);
    const float3 worldTangent = HitAttribute(vertexTangents, attr);
    const float3 eyePos       = frameCB.cameraPos.xyz;
    float3 lightAmbient       = frameCB.lightAmbient.rgb; // Requires scaling by the ambient buffer.
    float3 lightDiffuse       = frameCB.lightDiffuse.rgb; // Requires attenuation by the bump shadowing factor.

    // Edit: We are now using interpolated normals and tangents.
    //hitNormal  = mul(hitNormal, (float3x3)ObjectToWorld4x3());
    //hitTangent = mul(hitTangent,(float3x3)ObjectToWorld4x3());	// Tangent does not require normalization.
    //hitTangent = normalize(mul(hitTangent,(float3x3)ObjectToWorld4x3()));
    //float3 normalW = mul(vertexNormals[0], (float3x3)ObjectToWorld4x3());

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
    const float3 xOffsetPoint = RayPlaneIntersection(worldPos, /*planeNormal*/ worldNormal, ddxRay);
    const float3 yOffsetPoint = RayPlaneIntersection(worldPos, /*planeNormal*/ worldNormal, ddyRay);

    const PartialDerivatives pd = GetPartialDerivatives(xOffsetPoint, yOffsetPoint, vertexPositions, vertexTexCoords, uv);	

    // Compute barycentrics 
    //float3 baryX = BarycentricCoordinates(xOffsetPoint, vertexPositions[0], vertexPositions[1], vertexPositions[2]);
    //float3 baryY = BarycentricCoordinates(yOffsetPoint, vertexPositions[0], vertexPositions[1], vertexPositions[2]);

    // Compute UVs and take the difference
    //float3x2 uvMatrix = float3x2(vertexTexCoords);
    //float2 ddxUV = mul(baryX, uvMatrix) - uv;
    //float2 ddyUV = mul(baryY, uvMatrix) - uv;

    float3 albedo            = AlbedoMap.SampleGrad(AnisoWrap, uv, pd.ddxUV, pd.ddyUV);
    const float2 localNormal = NormalMap.SampleGrad(AnisoWrap, uv, pd.ddxUV, pd.ddyUV);
    //const float3 localNormal = NormalMap.SampleGrad(AnisoWrap, uv, pd.ddxUV, pd.ddyUV);
    const float4 rma	     = RMAMap.SampleGrad(AnisoWrap, uv, pd.ddxUV, pd.ddyUV);
    //float3 albedo_srgb = AlbedoMap.SampleLevel(AnisoClamp, uv, 0);
    //float3 localNormal = NormalMap.SampleLevel(AnisoClamp, uv, 0);
    //float3 rma_srgb = RMAMap.SampleLevel(AnisoClamp, uv, 0);
    const float3 emissive    = inst.material.emissive.rgb;

    albedo  = RemoveSRGBCurve(albedo);    // Convert srgb texture samples to linear color space.
    //rma.rgb	= RemoveSRGBCurve(rma.rgb);
    //Texture2D<float3> EmissiveMap
    //if (objData.material.emissiveTexIndex)
    //{
    //	Texture2D<float3> EmissiveMap = ResourceDescriptorHeap[objData.material.emissiveTexIndex];
    //	float3 emissive_srgb = EmissiveMap.SampleLevel(AnisoClamp, uv, 0);
    //	float3 emissive = RemoveSRGBCurve(emissive_srgb);
    //}
    //float3 texDiffuse_srgb = DiffuseMap.SampleLevel(AnisoClamp, uv, 0).rgb;
    //float3 texDiffuse = RemoveSRGBCurve(texDiffuse_srgb); // Convert texture sample to linear color space.
    //float3 texNormal = NormalMap.SampleLevel(AnisoClamp, uv, 0).rgb;

    const float3 V = normalize(eyePos - worldPos);  // View vector.
    const float3 L = -frameCB.lightDir;						// Light vector ("to light" opposite of light's direction).
    const float3 N = NormalSampleToWorldSpace(localNormal, worldNormal, worldTangent);
    //float3 N = PeturbNormal(localNormal, hitPosition, normalW, uv);

    // Before lighting, peturb the surface's normal by the one given in normal map.
    //float3 localNormal = TwoChannelNormalX2(NormalTexture.Sample(SurfaceSampler, pin.TexCoord).xy);

    // SampleGrad textures each triangle with a single color!
    //float3 texColor = DiffuseMap.SampleGrad(AnisoClamp, uv, 0 /*ddxUV*/, 0 /*ddyUV*/).rgb;
    //float3 texColor = DiffuseMap.SampleGrad(AnisoClamp, uv, ddxUV, ddyUV).rgb;

    /*
    // Trace a shadow ray.
    // Perform before tracing a reflection ray, because reflection rays increase the ray bounce count.
    // To eliminate shadow terminator artifacts, we calculate a new point for the shadow ray origin.
    const float3 shadowOrigin  = GetOffsetShadowRayOrigin(worldPos, vertexPositions, vertexNormals, attr);

    // Also offset the recalculated shadow ray origin to avoid self-intersection.
    float3 offsetOrigin = GetOffsetRayOrigin(shadowOrigin, worldNormal);    // offsetPos will be recalculated for reflection rays below.
    const Ray shadowRay    = { offsetOrigin, L };
    //const Ray shadowRay    = { shadowOrigin, L };
    //const Ray shadowRay    = { worldPos, L };
    const Ray ddxShadowRay = { xOffsetPoint, L };
    const Ray ddyShadowRay = { yOffsetPoint, L };

    // Update payload with auxilliary shadow rays.
    payload.ddxRay = (AuxilliaryRay)ddxShadowRay;
    payload.ddyRay = (AuxilliaryRay)ddyShadowRay;

    TraceShadowRayAndReportIfHit(shadowRay, payload);

    const uint shadowMask = (uint)payload.isShadowRayMiss;
    //const bool isShadowRayHit = payload.isShadowRayHit;
    //const bool isShadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, payload);
    //const bool isShadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, ddxShadowRay, ddyShadowRay, payload.recursionDepth);
    //bool shadowRayHit = TraceShadowRayAndReportIfHit(shadowRay, rayPayload.recursionDepth);
    */

    // Trace a reflection ray if surface is shiny enough.
    float3 reflectionColor = 0;

    if (rma.g < 0.5f)
    {
        const float3 R = reflect(WorldRayDirection(), N);

        // Calculate the reflection ray origin offset to avoid self-intersection.
        const float3 offsetOrigin = GetOffsetRayOrigin(worldPos, worldNormal);
        const Ray reflectionRay    = { offsetOrigin, R };
        //const Ray reflectionRay    = { worldPos, R };
        const Ray ddxReflectionRay = { xOffsetPoint, R };
        const Ray ddyReflectionRay = { yOffsetPoint, R };
        //Ray reflectionRay = { hitPosition, reflect(WorldRayDirection(), N) };

        // Update payload with auxilliary reflection rays.
        payload.ddxRay = ddxReflectionRay;
        payload.ddyRay = ddyReflectionRay;

        TraceRadianceRay(reflectionRay, payload);

        reflectionColor = payload.color;
        //const float3 reflectionColor = TraceRadianceRay(reflectionRay, ddxReflectionRay, ddyReflectionRay, payload.recursionDepth);
        //float3 reflectionColor = TraceRadianceRay(reflectionRay, rayPayload.recursionDepth);
        //float3 fresnelR = FresnelReflectanceSchlick(WorldRayDirection(), normalW, objectCB.material.albedo);
        //reflectedColor = objectCB.material.reflectanceCoef * fresnelR * reflectionColor;
    }

    // The occlusion buffers are quarter resolution, so we use bilinear sampling.
    const float2   uvCurr    = GetTextureCoords();
    const float4x4 prevWorld = prevFrameData.world;
    const float4x4 prevViewProjT = frameCB.prevViewProjTex;
    const float2   uvPrev  = CalculatePrevFrameTexCoords(worldPos, prevWorld, prevViewProjT);

    const float  ambientCurr = AmbientBufferCurr.SampleLevel(LinearClamp, uvCurr, 0);
    //const float  ambientPrev = AmbientBufferPrev.SampleLevel(LinearClamp, uvPrev, 0);
    // We also perform cheap temporal filtering by averaging with the previous frame value.
    const float  ambientFactor = ambientCurr; // lerp(ambientCurr, ambientPrev, 0.5f);

    lightAmbient *= ambientFactor;  // Scale the ambient lighting term by the ambient factor.

    // Sample the shadow screen mask.
    const float shadowCurr = ShadowBufferCurr.SampleLevel(LinearClamp, uvCurr, 0);
    //const float shadowPrev = ShadowBufferPrev.SampleLevel(LinearClamp, uvPrev, 0);
    // We also perform cheap temporal filtering by averaging with the previous frame value.
    const float shadowFactor = shadowCurr; // lerp(shadowCurr, shadowPrev, 0.5f);

    // Attenuate diffuse light intensity by bump shadowing factor.
    lightDiffuse *= BumpShadowingFactor(worldNormal, N, L);

    // Calculate physically based lighting.
    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel.
    float3 color = LightSurface(V, N, L, 1, lightDiffuse, albedo, rma.g, rma.b, rma.r, rma.a, lightAmbient,
        reflectionColor, shadowFactor); // isShadowRayHit);
        //, DiffIBLMap, SpecIBLMap);

    color += emissive;    // Add emissive lighting constribution.

    // Sample previous frame's output and blend with current frame color, except if this is the first rendered frame.
    if (frameCB.isFirstFrame)
    {
        payload.color = color;
        return;
    }

    const float3 prevColor = ColorBufferPrev.SampleLevel(LinearClamp, uvPrev, 0);
    color = lerp(color, prevColor, 0.5f);

    // Calculate final color.
    //float3 phongColor = CalculatePhongLighting(objData.material.diffuseColor, hitNormalW, shadowRayHit,
    //	frameCB.lightDir.rgb, frameCB.lightDiffuse.rgb, ambientLight,//frameCB.lightAmbient.rgb,
    //	objData.material.diffuseCoef, objData.material.specularCoef, objData.material.specularPower);

    //float3 color = texDiffuse * phongColor; // We haven't added reflection yet.
    //float3 color = texColor * (phongColor + reflectedColor);
    
    //rayPayload.color = color * sceneOcclusion; // apply scene RTAO.
    payload.color = color;
    //rayPayload.color = phongColor; // We're not applying reflected and texture color here.
}

//***************************************************************************
//*****************------ Any hit shaders-------************************
//***************************************************************************

// NB: Any hit shaders will be ignored if D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE flag is set in the application,
// unless overriden by D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE, or ray flag RAY_FLAG_FORCE_NON_OPAQUE.
//[shader("anyhit")]
//void AnyHitCubeShader(inout ColorRayPayload payload, Attributes attr)
//{
//	// Unused.
//}

[shader("anyhit")]
void AnyHitShader(inout ColorPayload payload, Attributes attr)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[InstanceID()+GeometryIndex()]; // Index into structured buffer with DXR intrinsics.

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
    const float3 vertexPositions[3] =
    {
        mul(float4(Vertices[indices[0]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[1]].position, 1), ObjectToWorld4x3()),
        mul(float4(Vertices[indices[2]].position, 1), ObjectToWorld4x3()),
    };
    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] =
    {
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

    // Vertex normal directions per triangle are identical, so no interpolation required.
    // Edit: We are now using interpolated normals and tangents.
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
    //float texAlpha = AlbedoMap.SampleLevel(AnisoClamp, uv, 0).a;

    if (texAlpha < 0.1f)
        IgnoreHit();
}

//***************************************************************************
//**********************------ Miss shaders -------**************************
//***************************************************************************

[shader("miss")]
void MissShader_CameraRay(inout ColorPayload payload)
//void MyMissShader(inout RayPayload payload : SV_RayPayload)
{
    const float3 rayDir = WorldRayDirection();

    TextureCube<float3> CubeMap = ResourceDescriptorHeap[SrvUAVs::EnvironmentMapSrv];
    //TextureCube<float3> CubeMap = ResourceDescriptorHeap[SrvUAVs::DiffuseIBLSrv];
    //TextureCube<float3> CubeMap = ResourceDescriptorHeap[SrvUAVs::SpecularIBLSrv];
    //TextureCube<float3> CubeMap = ResourceDescriptorHeap[SrvUAVs::CubeMapSrv];

    float3 color = CubeMap.SampleLevel(LinearClamp, rayDir, 0);
    color = RemoveSRGBCurve(color); // Convert to linear color space.

    // Negate light direction to calculate dot product between sun direction and ray direction.
    const float cosSunAngle = saturate(dot(rayDir, -frameCB.lightDir));
    const float interpolant = pow(cosSunAngle, 20);
    color = lerp(color, frameCB.lightDiffuse.rgb, interpolant);

    payload.color = color;

    // To slightly differentiate the raster and the raytracing, we will add a simple ramp color background by
    // modifying the `Miss` function: we simply obtain the
    // coordinates of the currently rendered pixel and use them to compute a linear gradient :
    //uint2 launchIndex = DispatchRaysIndex().xy;
    //float2 dims = DispatchRaysDimensions().xy;
    //float ramp = launchIndex.y / dims.y;
    //payload.color = float4(0.f, 0.2f, 0.7f - 0.3f * ramp, -1.f);
    
    //payload.colorAndDistance = float4(0.2f, 0.2f, 0.8f, -1);
    //payload.color = float4(0, 0, 0, 1);
}

[shader("miss")]
void MissShader_Occlusion(inout ColorPayload payload)
//void MyMissShader_ShadowRay(inout ShadowRayPayload payload : SV_RayPayload)
{
    //payload.isShadowRayMiss = true;
    //payload.isShadowRayHit = false;
}

//#endif // RAYTRACING_HLSL
