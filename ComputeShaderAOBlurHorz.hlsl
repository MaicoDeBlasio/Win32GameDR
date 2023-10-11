//=============================================================================
// SsaoBlurCS.hlsl adapted from Frank Luna (C) 2015 All Rights Reserved.
//
// Performs a bilateral edge preserving blur of the ambient map.  We use 
// a pixel shader instead of compute shader to avoid the switch from 
// compute mode to rendering mode.  The texture cache makes up for some of the
// loss of not having shared memory.  The ambient map uses 16-bit texture
// format, which is small, so we should be able to fit a lot of texels
// in the cache.
//=============================================================================

#define HLSL
#include "RaytracingHlslCompat.h"
//#include "Common.hlsli"

//Texture2D gInputMap  : register(t2); // override declaration in "Ssao.hlsli"
//RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<BlurConstants> cb0 : register(b0);
//ConstantBuffer<RtaoBlurWeights> cb0 : register(b0);
//ConstantBuffer<AoBlurDirection> cb1 : register(b1);

#define N 64
#define BlurRadius 5
#define CacheSize (N + 2 * BlurRadius)
groupshared float gCache[CacheSize];
//groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    // Unpack into float array.
    float blurWeights[12] =
    {
        cb0.blurWeights[0].x, cb0.blurWeights[0].y, cb0.blurWeights[0].z, cb0.blurWeights[0].w,
        cb0.blurWeights[1].x, cb0.blurWeights[1].y, cb0.blurWeights[1].z, cb0.blurWeights[1].w,
        cb0.blurWeights[2].x, cb0.blurWeights[2].y, cb0.blurWeights[2].z, cb0.blurWeights[2].w,
    };

    // Dynamic heap indexing introduced in SM6.6
    RWTexture2D<float> Input  = ResourceDescriptorHeap[cb0.blurMap0UavID];
    RWTexture2D<float> Output = ResourceDescriptorHeap[cb0.blurMap1UavID];
    Texture2D<float4> NormalDepthInput = ResourceDescriptorHeap[cb0.normalDepthMapSrvID];
    //RWTexture2D<float> Input  = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Uav];
    //RWTexture2D<float> Output = ResourceDescriptorHeap[SrvUAVs::AmbientMap1Uav];
    //Texture2D<float4> NormalDepthInput = ResourceDescriptorHeap[SrvUAVs::NormalDepthSrv];

    //RWTexture2D<float4> Input = ResourceDescriptorHeap[SrvUAVs::AmbientMap0Uav];
    //RWTexture2D<float4> Output = ResourceDescriptorHeap[SrvUAVs::AmbientMap1Uav];
    //RWTexture2D<float4> NormalDepthInput = ResourceDescriptorHeap[SrvUAVs::NormalDepthUav];

    // Added this code to fix 'G1A4676F8 no member named Length in Texture2D' error
    uint width, height; // , numMips;
    Input.GetDimensions(width, height);
    //gInputMap.GetDimensions(0u, width, height, numMips);
    uint2 size = uint2(width, height);

    // Equivalently using SampleLevel() instead of operator[]
    //uint x = dispatchThreadID.x;
    //uint y = dispatchThreadID.y;

    //float width  = 1.f / gInvRenderTargetSize.x;
    //float height = 1.f / gInvRenderTargetSize.y;

    //float u = (float)x / (float)width;
    //float v = (float)y / (float)height;

    //float2 texCoords = float2(u, v);

    //
    // Fill local thread storage to reduce bandwidth.  To blur 
    // N pixels, we will need to load N + 2*BlurRadius pixels
    // due to the blur radius.
    //

    // This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
    // have 2*BlurRadius threads sample an extra pixel.
    if (groupThreadID.x < BlurRadius)
    {
        // Clamp out of bound samples that occur at image borders.
        int x = max(dispatchThreadID.x - BlurRadius, 0);
        gCache[groupThreadID.x] = Input[int2(x, dispatchThreadID.y)];
    }
    if (groupThreadID.x >= N - BlurRadius)
    {
        // Clamp out of bound samples that occur at image borders.
        int x = min(dispatchThreadID.x + BlurRadius, size.x - 1);
        //int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x-1);
        gCache[groupThreadID.x + 2 * BlurRadius] = Input[int2(x, dispatchThreadID.y)];
    }

    // Clamp out of bound samples that occur at image borders.
    gCache[groupThreadID.x + BlurRadius] = Input[min(dispatchThreadID.xy, size.xy - 1)];
    //gCache[groupThreadID.x+gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy-1)];

    // Wait for all threads to finish.
    GroupMemoryBarrierWithGroupSync();

    int2 offset = int2(1, 0); // hardcoded horizontal blur offset
    //int2 offset;
    //float2 texOffset;
    //if (cb1.HorizontalBlur)
    //{
    //    offset = int2(1, 0);
    //    //texOffset = float2(gInvRenderTargetSize.x, 0.f);
    //}
    //else
    //{
    //    offset = int2(0, 1);
    //    //texOffset = float2(0.f, gInvRenderTargetSize.y);
    //}

    // The center value always contributes to the sum.
    float color = blurWeights[BlurRadius] * Input[dispatchThreadID.xy];
    //float4 color = blurWeights[BlurRadius] * Input[dispatchThreadID.xy];
    //float4 color = blurWeights[gBlurRadius] * gInputMap.SampleLevel(gsamPointClamp, texCoords, 0.f);
    //float4 color = blurWeights[gBlurRadius] * gInputMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0);
    float totalWeight = blurWeights[BlurRadius];

    // To do!
    float3 centerNormal = NormalDepthInput[dispatchThreadID.xy].xyz;
    float  centerDepth = NormalDepthInput[dispatchThreadID.xy].w;
    //float3 centerNormal = gNormalMap.SampleLevel(gsamPointClamp, texCoords, 0.f).xyz;
    //float  centerDepth = NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamDepthMap, texCoords, 0.f).r);
    
    //float3 centerNormal = gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz;
    //float  centerDepth = NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r);

    //
    // Now blur each pixel.
    //

    for (int i = -BlurRadius; i <= BlurRadius; ++i)
        //for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.x + BlurRadius + i;

        // We already added in the center weight.
        if (i == 0)
            continue;

        int2 blurOffset = dispatchThreadID.xy + i * offset;
        //float2 tex = texCoords + (float)i * texOffset;
        //float2 tex = pin.TexC + i * texOffset;

        // To do!
        float3 neighborNormal = NormalDepthInput[blurOffset].xyz;
        float  neighborDepth = NormalDepthInput[blurOffset].w;
        //float3 neighborNormal = gNormalMap.SampleLevel(gsamPointClamp, tex, 0.f).xyz;
        //float  neighborDepth = NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamDepthMap, tex, 0.f).r);

        //float3 neighborNormal = gNormalMap.SampleLevel(gsamPointClamp, tex, 0.0f).xyz;
        //float  neighborDepth = NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamDepthMap, tex, 0.0f).r);

        //
        // If the center value and neighbor values differ too much (either in 
        // normal or depth), then we assume we are sampling across a discontinuity.
        // We discard such samples from the blur.
        //

        if (dot(neighborNormal, centerNormal) >= 0.8f && abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = blurWeights[i + BlurRadius];

            // Add neighbor pixel to blur.
            color += weight * gCache[k];
            //color += weight * gInputMap.SampleLevel(gsamPointClamp, tex, 0.f);
            totalWeight += weight;
        }
    }

    // Compensate for discarded samples by making total weights sum to 1.
    Output[dispatchThreadID.xy] = color / totalWeight;
    //return color / totalWeight;
}
