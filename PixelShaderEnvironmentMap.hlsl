//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//=============================================================================
// Modified for DXTK12

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"

//TextureCube CubeMap : register(t6); // override declaration in Common.hlsli
//TextureCube CubeMap : register(t12);

//SamplerState LinearClamp : register(s1);

float4 main(float3 texCoord : TEXCOORD0) : SV_TARGET0
{

    TextureCube<float3> CubeMap = ResourceDescriptorHeap[SrvUAVs::EnvironmentMapSrv];

    const float3 rayDir = normalize(texCoord);
            
    float3 color = CubeMap.Sample(LinearClamp, rayDir);
    //float3 color = CubeMap.Sample(LinearClamp, normalize(texCoord));
    color = RemoveSRGBCurve(color); // Convert to linear color space.

    // Negate light direction to calculate dot product between sun direction and ray direction.
    const float cosSunAngle = saturate(dot(rayDir, -frameCB.lightDir));
    const float interpolant = pow(cosSunAngle, 20);
    color = lerp(color, frameCB.lightDiffuse.rgb, interpolant);

    // An fp16 render target format has a linear color space, so do not reapply the SRGB curve!
    return float4(color, 1);
}

//float4 PS(SkyVertexOut pin) : SV_Target
//{
//	return gCubeMap.Sample(LinearSampler, pin.PosL);
//}
