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

//ConstantBuffer<FrameConstants> frameCB : register(b0);
float4 PSMain(VertexOutput pin) : SV_TARGET
{
    Texture2D<float3> DiffuseMap = ResourceDescriptorHeap[SrvUAVs::RacetrackGrassAlbedoSrv];
    Texture2D<float3> NormalMap  = ResourceDescriptorHeap[SrvUAVs::RacetrackGrassNormalSrv];

    float3 diffuseColor_srgb = DiffuseMap.Sample(AnisoWrap, pin.texCoord * 10.f);
    //float3 diffuseColor_srgb = DiffuseMap.Sample(AnisoClamp, input.texCoord);
    float3 diffuseColor = RemoveSRGBCurve(diffuseColor_srgb); // Convert to linear color space.

    float3 texNormal = NormalMap.Sample(AnisoWrap, pin.texCoord * 10.f);
    //float3 texNormal = NormalMap.Sample(AnisoClamp, input.texCoord);
    
    // Interpolating normal can unnormalize it, so renormalize it.
    float3 normalW = normalize(pin.normalW);

    // Comment out to disable normal mapping.
     normalW = NormalSampleToWorldSpace(texNormal, normalW, pin.tangentW);

    // Directional light shading.
    float NdotL = dot(-frameCB.lightDir, normalW); // negate light direction
    float3 lightIntensity = frameCB.lightDiffuse.rgb * saturate(NdotL); // *input.color.rgb;

    // Determine if this surface pixel is in shadow with inline raytracing.
    // Instantiate ray query object.
    // Template parameter allows driver to generate a specialized
    // implementation.
    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[SrvUAVs::TLASSrv];

    // Set the ray's extents.
    RayDesc rayDesc;
    rayDesc.Origin = pin.posW;
    rayDesc.Direction = -frameCB.lightDir;
    // Set TMin to a nonzero value to avoid aliasing artifacts along contact areas.
    // Note: make sure to enable face culling so as to avoid surface face fighting.
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 10000;
                        
    float shadow = 1;

    RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

    // Set up a trace.  No work is done yet.
    q.TraceRayInline(
        Scene,
        RAY_FLAG_NONE, // OR'd with flags above
        0xFF,
        rayDesc);

    // Proceed() below is where behind-the-scenes traversal happens,
    // including the heaviest of any driver inlined code.
    // In this simplest of scenarios, Proceed() only needs
    // to be called once rather than a loop:
    // Based on the template specialization above,
    // traversal completion is guaranteed.
    q.Proceed();

    // Examine and act on the result of the traversal.
    // Was a hit committed?
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        shadow = 0.25f;

    float3 color = diffuseColor * shadow * (lightIntensity + frameCB.lightAmbient.rgb);
    //float3 color = diffuseColor * (lightIntensity + frameCB.lightAmbient.rgb); // tints ambient light with object diffuse color
    //float3 color = input.color.rgb * (lightIntensity + frameCB.lightAmbient.rgb);

    //float3 finalColor = RemoveSRGBCurve(color);

    // An fp16 render target format has a linear color space, so do not reapply the SRGB curve!
    return float4(color, 1);
    //return float4(finalColor, 1);
}
