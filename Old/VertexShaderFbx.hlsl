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

VertexOutput VSMain(VertexSkinnedInput vin)
{
    VertexOutput vout;

    // Perform vertex skinning.
    // Ignore input.boneWeights.w and instead calculate the last weight value to ensure all bone weights sum to unity.

    float4 weights = vin.boneWeights;
    //weights.w = 1.f - dot(weights.xyz, float3(1.f, 1.f, 1.f));

    weights.w = 1.f - weights.x - weights.y - weights.z;

    float3 posL = mul(float4(vin.posL, 1), boneCB.boneTransforms[vin.boneIndices.x]).xyz * weights.x;
    posL +=       mul(float4(vin.posL, 1), boneCB.boneTransforms[vin.boneIndices.y]).xyz * weights.y;
    posL +=       mul(float4(vin.posL, 1), boneCB.boneTransforms[vin.boneIndices.z]).xyz * weights.z;
    posL +=       mul(float4(vin.posL, 1), boneCB.boneTransforms[vin.boneIndices.w]).xyz * weights.w;

    float3 normalL = mul(vin.normalL, (float3x3)boneCB.boneTransforms[vin.boneIndices.x]) * weights.x;
    normalL +=       mul(vin.normalL, (float3x3)boneCB.boneTransforms[vin.boneIndices.y]) * weights.y;
    normalL +=       mul(vin.normalL, (float3x3)boneCB.boneTransforms[vin.boneIndices.z]) * weights.z;
    normalL +=       mul(vin.normalL, (float3x3)boneCB.boneTransforms[vin.boneIndices.w]) * weights.w;

    float3 tangentL = mul(vin.tangentL, (float3x3)boneCB.boneTransforms[vin.boneIndices.x]) * weights.x;
    tangentL +=       mul(vin.tangentL, (float3x3)boneCB.boneTransforms[vin.boneIndices.y]) * weights.y;
    tangentL +=       mul(vin.tangentL, (float3x3)boneCB.boneTransforms[vin.boneIndices.z]) * weights.z;
    tangentL +=       mul(vin.tangentL, (float3x3)boneCB.boneTransforms[vin.boneIndices.w]) * weights.w;
    // Ends vertex skinning.

    float4 posW = mul(float4(posL, 1), meshCB.world);
    vout.posW = posW.xyz;
    vout.posH = mul(posW, frameCB.viewProj);

    // Blending can output vertex normals of different length, which will skew interpolation if not normalized.
    vout.normalW = normalize(mul(normalL, (float3x3)meshCB.world));
    //result.normalW = mul(normalL, (float3x3)objectCB.world);
    vout.tangentW = mul(tangentL, (float3x3)meshCB.world);

    //result.normalW = input.normalL;
    vout.texCoord = vin.texCoord;
    //result.color = float3(1, 1, 1);

    return vout;
}
