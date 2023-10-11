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

// Original note from FXAA_WhitePaper:
// FXAA presets 0 through 2 require an anisotropic sampler with max anisotropy set to 4, and for all presets,
// there is a required rcpFrame constant which supplies the reciprocal of the inputTexture size in pixels:
// { 1.0f/inputTextureWidth, 1.0f/inputTextureHeight, 0.0f, 0.0f }

#define HLSL

// Quality preset 12 will be set by default if no other preset quality level is defined.
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39 // Highest possible quality.

#include "RaytracingHlslCompat.h"
#include "Common.hlsli"
#include "Fxaa3_11.hlsli"

float4 main(TexCoordVertexOutput pin) : SV_TARGET
{
    Texture2D<float4> InputTex = ResourceDescriptorHeap[SrvUAVs::TonemapSrv]; // Alpha channel contains luma value.
    FxaaTex tex = { LinearClamp, InputTex };

    uint width, height;
    InputTex.GetDimensions(width, height);

    float2 fxaaQualityRcpFrame     = float2( 1.f / (float) width,  1.f / (float) height);
    float  fxaaQualitySubpix       = 1.f;     // Amount of sub-pixel aliasing removal (upper limit - softest).
    //float  fxaaQualitySubpix       = 0.75f;     // Amount of sub-pixel aliasing removal (default).
    float  fxaaQualityEdgeThreshold= 0.125f;    // Minimum local contrast required to apply FXAA (high quality).
    //float  fxaaQualityEdgeThreshold= 0.166f;    // Minimum local contrast required to apply FXAA (default).
    float fxaaQualityEdgeThresholdMin = 0.0625f;// Trims FXAA from processing darks (high quality - faster).
    //float fxaaQualityEdgeThresholdMin = 0.0833f;// Trims FXAA from processing darks (default).

    return FxaaPixelShader(
                        pin.texCoord,               // pos
                        0,                          // unused on PC
                        tex,                        // tex
                        tex,                        // use same input as for tex
                        tex,                        // use same input as for tex
                        fxaaQualityRcpFrame,        // reciprocal texture size
                        0,                          // unused on PC
                        0,                          // unused on PC
                        0,                          // unused on PC
                        fxaaQualitySubpix,          // sub-pixel aliasing tuning
                        fxaaQualityEdgeThreshold,   // minimum local contrast required
                        fxaaQualityEdgeThresholdMin,// dark aliasing tuning
                        0, // unused on PC
                        0, // unused on PC
                        0, // unused on PC
                        0 // unused on PC
                        );
}
