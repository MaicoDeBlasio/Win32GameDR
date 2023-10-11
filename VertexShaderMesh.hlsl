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

VertexOutput main(VertexMeshInput vin)
{
    VertexOutput vout;

    float4 posW     = mul(float4(vin.posL, 1), meshCB.world);
    vout.posW       = posW.xyz;
    vout.posH       = mul(posW, frameCB.viewProj);
    vout.scrnPosH   = mul(posW, frameCB.viewProjTex);
    vout.normalW    = mul(vin.normalL, (float3x3)meshCB.world);
    vout.tangentW   = mul(vin.tangentL,(float3x3)meshCB.world);
    vout.texCoord   = vin.texCoord;
    vout.instanceID = meshCB.instanceID; // uint vertex attributes are not interpolated.
    //result.color = float3(1, 1, 1);

    return vout;
}
