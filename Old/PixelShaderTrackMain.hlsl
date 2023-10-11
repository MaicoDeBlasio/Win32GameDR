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
    Texture2D<float3> DiffuseMap = ResourceDescriptorHeap[SrvUAVs::RacetrackRoadAlbedoSrv];
    Texture2D<float3> NormalMap  = ResourceDescriptorHeap[SrvUAVs::RacetrackRoadNormalSrv];

    float3 diffuseColor_srgb = DiffuseMap.Sample(AnisoClamp, pin.texCoord);
    float3 diffuseColor = RemoveSRGBCurve(diffuseColor_srgb); // Convert to linear color space.

    float3 texNormal = NormalMap.Sample(AnisoClamp, pin.texCoord);
    
    // Interpolating normal can unnormalize it, so renormalize it.
    float3 normalW = normalize(pin.normalW);

    // Comment out to disable normal mapping.
    normalW = NormalSampleToWorldSpace(texNormal, normalW, pin.tangentW);

    // Directional light shading.
    float NdotL = dot(-frameCB.lightDir, normalW); // negate light direction
    float3 lightIntensity = frameCB.lightDiffuse.rgb * saturate(NdotL); // *input.color.rgb;

    float3 color = diffuseColor * (lightIntensity + frameCB.lightAmbient.rgb); // tints ambient light with object diffuse color
    //float3 color = input.color.rgb * (lightIntensity + frameCB.lightAmbient.rgb);

    //float3 finalColor = RemoveSRGBCurve(color);

    // An fp16 render target format has a linear color space, so do not reapply the SRGB curve!
    return float4(color, 1);
    //return float4(finalColor, 1);
}
