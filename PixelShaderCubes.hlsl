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

// Christian Schuler, "Normal Mapping without Precomputed Tangents", ShaderX 5, Chapter 2.6, pp. 131-140
// See also follow-up blog post: http://www.thetenthplanet.de/archives/1180
float3x3 CalculateTBN(float3 p, float3 n, float2 tex)
{
    float3 dp1 = ddx(p);
    float3 dp2 = ddy(p);
    float2 duv1 = ddx(tex);
    float2 duv2 = ddy(tex);

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    float3 t = mul(float2(duv1.x, duv2.x), inverseM);
    float3 b = mul(float2(duv1.y, duv2.y), inverseM);
    return float3x3(t, b, n);
}

float3 PeturbNormal(float3 localNormal, float3 position, float3 normal, float2 texCoord)
{
    const float3x3 TBN = CalculateTBN(position, normal, texCoord);
    return mul(localNormal, TBN);
}

//float4 PSMain(PSInput input) : SV_TARGET
float4 main(VertexOutput pin) : SV_TARGET
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[SrvUAVs::InstanceBufferSrv];
    InstanceData inst = Instances[pin.instanceID];

    Texture2D<float3> AlbedoMap     = ResourceDescriptorHeap[inst.material.albedoTexIndex];
    Texture2D<float3> NormalMap     = ResourceDescriptorHeap[inst.material.normalTexIndex];
    Texture2D<float3> EmissiveMap   = ResourceDescriptorHeap[inst.material.emissiveTexIndex];
    Texture2D<float4> RMAMap        = ResourceDescriptorHeap[inst.material.RMATexIndex];
    Texture2D<float>  AmbientBuffer = ResourceDescriptorHeap[SrvUAVs::AmbientBuffer0Srv];
    Texture2D<float>  ShadowBuffer  = ResourceDescriptorHeap[SrvUAVs::ShadowBuffer0Srv];

    const float3 worldPos    = pin.posW;
    const float3 worldNormal = pin.normalW;   // No interpolation because all vertex normals are identical.
    const float2 uv          = pin.texCoord;
    const float3 eyePos      = frameCB.cameraPos.xyz;
    const float3 lightDiffuse= frameCB.lightDiffuse.rgb;
    float3 lightAmbient      = frameCB.lightAmbient.rgb;   // Requires scaling by the projected ambient screenspace buffer.

    float3 albedo           = AlbedoMap.Sample(AnisoClamp, uv);
    const float3 localNormal= NormalMap.Sample(AnisoClamp, uv);
    const float3 emissive   = EmissiveMap.Sample(AnisoClamp, uv);
    const float4 rma	    = RMAMap.Sample(AnisoClamp, uv);

    albedo = RemoveSRGBCurve(albedo) * inst.material.albedo.rgb; // Modulate albedo by instance's color and convert to linear color space.

    const float3 V = normalize(eyePos - worldPos);// View vector.
    const float3 L = -frameCB.lightDir;						// Light vector ("to light" opposite of light's direction).
    const float3 N = PeturbNormal(localNormal, worldPos, worldNormal, uv);

    // Finish texture projection and scale the ambient lighting term by the RTAO map.
    // The RTAO and shadow screen space maps are in quarter resolution, so use bilinear sampling.
    pin.scrnPosH /= pin.scrnPosH.w;
    const float ambientFactor = AmbientBuffer.Sample(LinearClamp, pin.scrnPosH.xy, 0);
    lightAmbient *= ambientFactor;

    const float shadowFactor = ShadowBuffer.Sample(LinearClamp, pin.scrnPosH.xy, 0);

    // Calculate physically based lighting.
    // glTF2 defines metalness as B channel, roughness as G channel, and occlusion as R channel.
    float3 color = LightSurface(V, N, L, 1, lightDiffuse, albedo, rma.g, rma.b, rma.r, rma.a, lightAmbient,
        0, shadowFactor); // We don't apply reflection color to cube shading.

    color += emissive;    // Add emissive lighting constribution.

    //float3 diffuseColor = DiffuseMap.Sample(AnisoClamp, input.texCoord);
    //diffuseColor = RemoveSRGBCurve(diffuseColor); // Convert to linear color space.

    // Directional light shading.
    //float NdotL = dot(-frameCB.lightDir, input.normalW); // negate light direction
    //float3 lightIntensity = frameCB.lightDiffuse.rgb * saturate(NdotL);// *input.color.rgb;
    //float3 diffuseColor = frameCB.lightDiffuse.rgb * saturate(NdotL);// *input.color.rgb;

    //float3 color = diffuseColor * (lightIntensity + frameCB.lightAmbient.rgb); // Tints ambient light with object diffuse color.
    //float3 color = input.color.rgb * (diffuseColor + frameCB.lightAmbient.rgb);

    //float3 color_srgb = ApplySRGBCurve(color);
    //float3 finalColor = RemoveSRGBCurve(color);
    //float3 color = RemoveSRGBCurve(input.normalW);
    //float3 color = RemoveSRGBCurve(input.color.rgb);
    //float3 color = RemoveSRGBCurve(float3(1, 0, 0));

    // An fp16 render target format has a linear color space, so do not reapply the SRGB curve!
    return float4(color, 1);
    //return float4(finalColor, 1);
    //return input.color;
}
