//=============================================================================
// ComputeShaderFinalPostProcess.hlsl
//
// Performs a final postprocess step by blending samples from the blur, bloom,
// and original textures, and applies a depth of field effect based on the ray
// distances stored in the normal-depth texture.
//=============================================================================

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"

float3 DistanceDOF(in float3 colorFocus, in float3 colorBlurred, in float rayDistance)
{
    // Find the depth based blur factor with a distance offset and a lerp coefficient to set blur rate
    const float blurFactor = saturate(0.1f * (rayDistance - 10.f)); // hard coded DoF strength and distance offset

    // Lerp with the blurred color based on the CoC factor
    return lerp(colorFocus, colorBlurred, blurFactor);
}

#define N 8

[numthreads(N, N, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Quarter-resolution textures.
    //Texture2D<float>   AmbientMap = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Srv];
    Texture2D<float3>  BloomTex = ResourceDescriptorHeap[SrvUAVs::BloomSrv];
    Texture2D<float3>  BlurTex  = ResourceDescriptorHeap[SrvUAVs::Pass2Srv];
    Texture2D<float4>  DepthTex = ResourceDescriptorHeap[SrvUAVs::NormalDepthSrv];
    // Full resolution textures. 
    Texture2D<float3>  HdrTex   = ResourceDescriptorHeap[SrvUAVs::HdrSrv];
    RWTexture2D<float4>Output   = ResourceDescriptorHeap[SrvUAVs::DxrUav];

    // Get texture coordinates for a full resolution texture.
    uint width, height; // , numMips;
    Output.GetDimensions(width, height);

    const float u = (float) dispatchThreadID.x / (float) width;
    const float v = (float) dispatchThreadID.y / (float) height;

    const float2 uv = float2(u, v);

    // Get samples from each texture.
    //const float  ambientAccess = AmbientMap.SampleLevel(LinearClamp, uv, 0);
    const float3 colorBloom = BloomTex.SampleLevel(LinearClamp, uv, 0);
    const float3 colorBlur  = BlurTex.SampleLevel(LinearClamp, uv, 0);
    const float3 colorHdr   = HdrTex.SampleLevel(PointClamp, uv, 0); // Fullsize texture.

    // Depth channel of normal-depth texture stores hit ray distances, so no linear conversion required.
    const float d = DepthTex.SampleLevel(LinearClamp, uv, 0).w;

    float3 colorOutput = DistanceDOF(colorHdr, colorBlur, d);  // Lerp the output color between focused and blurred textures.

    colorOutput = max(colorBloom, colorOutput);         // Output the bloom contribution.

    //colorOutput *= ambientAccess;      // Modulate output color with ambient value.

    // Calculate luma in perceptual color space (gamma 2.2) and store in alpha channel.
    const float  luma   = dot(colorOutput, float3(0.299f, 0.587f, 0.114f));
    const float3 luma22 = ApplySRGBCurve(float3(luma.rrr));
    //float luma = sqrt(dot(colorOutput, float3(0.299f, 0.587f, 0.114f)));

    // Output RGBL value.
    Output[dispatchThreadID.xy] = float4(colorOutput, luma22.r);
    //Output[dispatchThreadID.xy] = float4(colorOutput, luma);
    //Output[dispatchThreadID.xy] = float4(colorOutput, 1);

    // Test outputs.
    //Output[dispatchThreadID.xy] = float4(luma.rrr, 1);
    //return float4(distortCoords, 0.f, 1.f);
    //return float4(colorHDR, 1.f);
    //return float4(colorBlurred, 1.f);
    //return float4(colorBloom, 1.f);
    //return float4(colorPass2, 1.f);
    //return float4((p.z * 0.01f).rrr, 1.f); // multiply by a contrast factor
    //return float4(colorNormals, 1.f);
    //return float4(colorAmbient.rrr, 1.f);
}
