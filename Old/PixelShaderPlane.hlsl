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

//SamplerState AnisoWrap : register(s0);
//Texture2D DiffuseMap : register(t0);

//float4 PSMain(PSInput input) : SV_TARGET
float4 PSMain(VertexOutput input) : SV_TARGET
{
    Texture2D DiffuseMap = ResourceDescriptorHeap[SrvUAVs::TreeAlbedoSrv];
    //Texture2D DiffuseMap = ResourceDescriptorHeap[SrvUAVs::PlaneDiffuseSrv]; // introduced in SM6.6
    //Texture2D<float4> DiffuseMap = ResourceDescriptorHeap[index];
    float4 diffuseColor_srgb = DiffuseMap.Sample(AnisoClamp, input.texCoord);
    float3 diffuseColor = RemoveSRGBCurve(diffuseColor_srgb.rgb); // Convert to linear color space.

    // Directional light shading.
    float  NdotL = dot(-frameCB.lightDir, input.normalW); // negate light direction
    float3 lightIntensity = frameCB.lightDiffuse.rgb * saturate(NdotL);// *input.color.rgb;

    float3 color = diffuseColor * (lightIntensity + frameCB.lightAmbient.rgb); // Tints ambient light with object diffuse color.
    //float3 color = diffuseColor.rgb * (lightIntensity + frameCB.lightAmbient.rgb);
    //float3 finalColor = input.color.rgb * (lightIntensity + frameCB.lightAmbient.rgb);

    //float3 color_srgb = ApplySRGBCurve(color); // Convert to srgb color space.
    //float3 finalColor = RemoveSRGBCurve(color);
    //float3 color = RemoveSRGBCurve(input.normalW);
    //float3 color = RemoveSRGBCurve(input.color.rgb);
    //float3 color = RemoveSRGBCurve(float3(1, 0, 0));

    // An fp16 render target format has a linear color space, so do not reapply the SRGB curve!
    return float4(color, diffuseColor_srgb.a); // This is a transparent texure!
    //return float4(color_srgb, diffuseColor_srgb.a);
    //return float4(finalColor, diffuseColor.a);
    //return float4(diffuseColor, 1);
    //return input.color;
}
