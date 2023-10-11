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

//ConstantBuffer<FrameConstants>  frameCB : register(b0);
//ConstantBuffer<ObjectConstants> objectCB : register(b1);

//PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
//VertexOutput VSMain(float3 posL : SV_POSITION)
VertexOutput VSMain(VertexPlaneInput input)
{
    VertexOutput result;

    float4 posW = float4(input.posL, 1); // no world transformation at present
    result.posW = posW.xyz;
    result.posH = mul(posW, frameCB.viewProj);

    result.normalW = input.normalL; // no world transform required
    result.texCoord = input.texCoord;
    //result.color = float3(1, 1, 1);
    //result.color = objectCB.material.albedo;
    //result.color = objectCB.diffuse;
    //result.color = input.color; // grey

    return result;
}
