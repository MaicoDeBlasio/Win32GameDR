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

VertexOutput main(VertexCubeInput vin, uint instanceID : SV_InstanceID)
//VertexOutput VSMain(VertexCubeInput input, uint instance_id : SV_InstanceID, uint vertex_id : SV_VertexID)
//PSInput VSMain(float4 position : SV_POSITION, float4 color : COLOR)
{
    VertexOutput vout;

    float3 posW   = mul(float4(vin.posL, 1), vin.world); // Result of this matrix mult returns a float3.
    //float4 posW = float4(input.posL, 1); // no world transformation at present
    vout.posW     = posW;
    vout.posH     = mul(float4(posW, 1), frameCB.viewProj);
    vout.scrnPosH = mul(float4(posW, 1), frameCB.viewProjTex);
    //result.posW = posW.xyz;
    //result.posH = mul(posW, cameraCB.viewProj);

    // Use SV_VertexID and SV_InstanceID system values to determine color for first vertex.
    //if (vertex_id == 0)
    //{
    //    switch (instance_id)
    //    {
    //    case 0:
    //        result.color = float4(1, 0, 0, 1); // red
    //        break;
    //    case 1:
    //        result.color = float4(0, 1, 0, 1); // green
    //        break;
    //    case 2:
    //        result.color = float4(0, 0, 1, 1); // blue
    //        break;
    //    }
    //}
    //else
    //    result.color = input.color;
    
    vout.normalW    = mul(vin.normalL,(float3x3)vin.world);
    vout.texCoord   = vin.texCoord;
    //result.color    = input.instColor;
    vout.instanceID = instanceID; // uint vertex attributes are not interpolated.
    //result.color = input.color * input.instColor;
    //result.position = position;

    return vout;
}
