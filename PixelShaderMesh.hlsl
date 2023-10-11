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
#include "CommonPBR.hlsli"

//ConstantBuffer<FrameConstants> frameCB : register(b0);

const bool AlphaTest(uint instanceID, uint primitiveID, float2 barycentrics)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[instanceID];
    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, primitiveID, Indices);

    // Retrieve corresponding texcoords for the triangle vertices.
    const float2 vertexTexCoords[3] =
    {
        Vertices[indices[0]].texCoord,
        Vertices[indices[1]].texCoord,
        Vertices[indices[2]].texCoord,
    };

    const float2 uv = HitAttribute(vertexTexCoords, (Attributes)barycentrics);

    Texture2D<float4> AlbedoMap = ResourceDescriptorHeap[inst.material.albedoTexIndex];
    const float texAlpha =  AlbedoMap.Sample(AnisoClamp, uv).a;

    return texAlpha >= 0.1f;
}

float4 main(VertexOutput pin, uint primitiveID : SV_PrimitiveID) : SV_TARGET
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[frameCB.instBufferSrvID];
    //StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[pin.instanceID];
    //ObjectData objData = ObjData[meshCB.instanceID];

    Texture2D<float4> AlbedoMap     = ResourceDescriptorHeap[inst.material.albedoTexIndex]; // Must sample alpha channel.
    Texture2D<float2> NormalMap     = ResourceDescriptorHeap[inst.material.normalTexIndex]; // In prep for sampling a BC5_SNORM texture.
    //Texture2D<float3> NormalMap     = ResourceDescriptorHeap[inst.material.normalTexIndex];
    //Texture2D<float3> EmissiveMap = ResourceDescriptorHeap[objData.material.emissiveTexIndex];
    Texture2D<float4> RMAMap        = ResourceDescriptorHeap[inst.material.RMATexIndex];
    Texture2D<float>  AmbientBuffer = ResourceDescriptorHeap[SrvUAVs::AmbientBuffer0Srv];
    Texture2D<float>  ShadowBuffer  = ResourceDescriptorHeap[SrvUAVs::ShadowBuffer0Srv];

    const float3 worldPos     = pin.posW;
    const float3 worldNormal  = normalize(pin.normalW);     // Interpolation can unnormalize normal, so renormalize it.
    const float2 uv           = pin.texCoord * inst.material.textureWrapFactor;
    const float3 worldTangent = pin.tangentW;
    const float3 eyePos       = frameCB.cameraPos.xyz;
    float3 lightAmbient       = frameCB.lightAmbient.rgb;   // Requires scaling by the projected ambient screenspace buffer.
    float3 lightDiffuse       = frameCB.lightDiffuse.rgb;   // Requires attenuation by the bump shadowing factor.

    // Discard pixel if texture alpha < 0.1.
    // We do this test as soon as possible in the shader so that we can potentially exit the shader early.
    // This test also eliminates ghost polygons around transparent geometry.
    float4 albedo = AlbedoMap.Sample(AnisoWrap, uv);
	clip(albedo.a - 0.1f);

    const float2 localNormal = NormalMap.Sample(AnisoWrap, uv);
    //const float3 localNormal = NormalMap.Sample(AnisoWrap, uv);
    //const float3 emissive = EmissiveMap.Sample(AnisoClamp, uv);
    const float4 rma	     = RMAMap.Sample(AnisoWrap, uv);
    const float3 emissive    = inst.material.emissive.rgb;

    albedo.rgb = RemoveSRGBCurve(albedo.rgb);   // Convert albedo to linear color space.

    const float3 V = normalize(eyePos - worldPos);// View vector.
    const float3 L = -frameCB.lightDir;						// Light vector ("to light" opposite of light's direction).
    const float3 N = NormalSampleToWorldSpace(localNormal, worldNormal, worldTangent);     // Comment out to disable normal mapping.
    //float3 N = PeturbNormal(normal, litPosition, worldNormal, uv);

    // Finish texture projection and scale the ambient lighting term by the RTAO map.
    // The RTAO and shadow screen space maps are in quarter resolution, so use bilinear sampling.
    pin.scrnPosH /= pin.scrnPosH.w;
    const float ambientFactor = AmbientBuffer.Sample(LinearClamp, pin.scrnPosH.xy, 0);
    lightAmbient *= ambientFactor;

    const float shadowFactor = ShadowBuffer.Sample(LinearClamp, pin.scrnPosH.xy, 0);

    /*

    // Determine if this surface pixel is in shadow with inline raytracing.
    // Instantiate ray query object.
    // Template parameter allows driver to generate a specialized implementation.
    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[frameCB.tlasBufferSrvID];
    //RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SrvUAVs::TLASSrv];

    // To eliminate shadow terminator artifacts, we calculate a new point for the shadow ray origin.
    StructuredBuffer<VertexPosNormalTexTangent> Vertices = ResourceDescriptorHeap[inst.geometryData.vertexBufferIndex];
    ByteAddressBuffer Indices = ResourceDescriptorHeap[inst.geometryData.indexBufferIndex];

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    const uint3 indices = LoadIndices(inst.geometryData.isIndex16, primitiveID, Indices);

    // Retrieve corresponding vertex positions for the triangle vertices.
    const float3 vertexPositions[3] = {
        mul(float4(Vertices[indices[0]].position, 1), (float4x3)meshCB.world),
        mul(float4(Vertices[indices[1]].position, 1), (float4x3)meshCB.world),
        mul(float4(Vertices[indices[2]].position, 1), (float4x3)meshCB.world),
    };

    // Retrieve corresponding vertex normals for the triangle vertices.
    const float3 vertexNormals[3] = {
        mul(Vertices[indices[0]].normal, (float3x3)meshCB.world),
        mul(Vertices[indices[1]].normal, (float3x3)meshCB.world),
        mul(Vertices[indices[2]].normal, (float3x3)meshCB.world),
    };

    const float3 shadowOrigin  = GetOffsetShadowRayOrigin(worldPos, vertexPositions, vertexNormals);

    // Also offset the recalculated shadow ray origin to avoid self-intersection.
    const float3 offsetOrigin = GetOffsetRayOrigin(shadowOrigin, worldNormal);

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = offsetOrigin; //shadowOrigin;
    //rayDesc.Origin = worldPosition;
    rayDesc.Direction = L;
    // Set TMin to a nonzero value to avoid aliasing artifacts along contact areas.
    // Note: make sure to enable face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0; //SurfaceEpsilon;
    rayDesc.TMax = 100;

    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;
    //RayQuery < RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

    // Set up a trace. No work is done yet.
    q.TraceRayInline(
        Scene,
        RAY_FLAG_NONE, // OR'd with flags above
        0xFF,
        rayDesc);

    // Proceed() below is where behind-the-scenes traversal happens, including the heaviest of any driver inlined code.
    // In this simplest of scenarios, Proceed() only needs to be called once rather than a loop:
    // Based on the template specialization above, traversal completion is guaranteed.
    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            if ( AlphaTest(
                 q.CandidateInstanceID() + q.CandidateGeometryIndex(),
                 q.CandidatePrimitiveIndex(),
                 q.CandidateTriangleBarycentrics()) )
            {
                q.CommitNonOpaqueTriangleHit();
            }
        }
    }

    // Examine and act on the result of the traversal.
    const uint shadowMask = (uint)!(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);
    //const bool inShadow = (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT);

    */

    // Attenuate diffuse light intensity by bump shadowing factor.
    lightDiffuse *= BumpShadowingFactor(worldNormal, N, L);

    // Calculate physically based lighting.
    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel.
    float3 color = LightSurface(V, N, L, 1, lightDiffuse, albedo.rgb, rma.g, rma.b, rma.r, rma.a, lightAmbient,
        0, shadowFactor); //inShadow);

    color += emissive;    // Add emissive lighting constribution.

    //Texture2D<float3> DiffuseMap = ResourceDescriptorHeap[SrvUAVs::SuzanneAlbedoSrv];
    //Texture2D<float3> NormalMap  = ResourceDescriptorHeap[SrvUAVs::SuzanneNormalSrv];

    //float3 diffuseColor = DiffuseMap.Sample(AnisoClamp, input.texCoord);
    //diffuseColor = RemoveSRGBCurve(diffuseColor); // Convert to linear color space.

    //float3 texNormal = NormalMap.Sample(AnisoClamp, input.texCoord);
    
    // Interpolating normal can unnormalize it, so renormalize it.
    //float3 normalW = normalize(input.normalW);

    // Comment out to disable normal mapping.
    //normalW = NormalSampleToWorldSpace(texNormal, normalW, input.tangentW);

    // Directional light shading.
    //float NdotL = dot(-frameCB.lightDir, normalW); // negate light direction
    //float3 lightIntensity = frameCB.lightDiffuse.rgb * saturate(NdotL);// *input.color.rgb;

    //float3 color = diffuseColor * (lightIntensity + frameCB.lightAmbient.rgb); // tints ambient light with object diffuse color
    //float3 color = input.color.rgb * (lightIntensity + frameCB.lightAmbient.rgb);

    //float3 finalColor = RemoveSRGBCurve(color);

    // An fp16 render target format has a linear color space, so do not reapply the SRGB curve!
    return float4(color, albedo.a);
    //return float4(finalColor, 1);
}
