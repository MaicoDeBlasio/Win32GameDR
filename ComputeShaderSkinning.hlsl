//=============================================================================
// VertexSkinningCS.hlsl by Maico De Blasio (C) 2023 All Rights Reserved.
//
// Performs vertex skinning of bone rigged FBX models for raytraced animation.
//=============================================================================

#define HLSL
#include "RaytracingHlslCompat.h"
//#include "Common.hlsli"

ConstantBuffer<BoneConstants> boneCB : register(b1);

#define N 64

[numthreads(N, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // Dynamic heap indexing introduced in SM6.6
    StructuredBuffer<VertexFbxBones> Input = ResourceDescriptorHeap[SrvUAVs::DoveVertexBufferSrv];
    RWStructuredBuffer<VertexPosNormalTexTangent> Output = ResourceDescriptorHeap[SrvUAVs::DoveSkinnedVertexBufferUav];

    const float3 InPos         = Input[dispatchThreadID.x].position;
    const float3 InNormal      = Input[dispatchThreadID.x].normal;
    const float2 InTexCoord    = Input[dispatchThreadID.x].texCoord;
    const float3 InTangent     = Input[dispatchThreadID.x].tangent;
    const uint   packedIndices = Input[dispatchThreadID.x].boneIndices;
    float4       InWeights     = Input[dispatchThreadID.x].boneWeights;

    // Ignore input.boneWeights.w and instead calculate the last weight value to ensure all bone weights sum to unity.
    InWeights.w = 1.f - InWeights.x - InWeights.y - InWeights.z;

    // Manually unpack bone indices.
    uint4 InBoneIndices = 0;
    InBoneIndices.x = (packedIndices >> 0) & 0x000000ff;
    InBoneIndices.y = (packedIndices >> 8) & 0x000000ff;
    InBoneIndices.z = (packedIndices >> 16) & 0x000000ff;
    InBoneIndices.w = (packedIndices >> 24) & 0x000000ff;

    // Do vertex skinning.
    float3 OutPos = mul(float4(InPos, 1), boneCB.boneTransforms[InBoneIndices.x]).xyz * InWeights.x;
    OutPos += mul(float4(InPos, 1), boneCB.boneTransforms[InBoneIndices.y]).xyz * InWeights.y;
    OutPos += mul(float4(InPos, 1), boneCB.boneTransforms[InBoneIndices.z]).xyz * InWeights.z;
    OutPos += mul(float4(InPos, 1), boneCB.boneTransforms[InBoneIndices.w]).xyz * InWeights.w;

    float3 OutNormal = mul(InNormal, (float3x3)boneCB.boneTransforms[InBoneIndices.x]) * InWeights.x;
    OutNormal += mul(InNormal, (float3x3)boneCB.boneTransforms[InBoneIndices.y]) * InWeights.y;
    OutNormal += mul(InNormal, (float3x3)boneCB.boneTransforms[InBoneIndices.z]) * InWeights.z;
    OutNormal += mul(InNormal, (float3x3)boneCB.boneTransforms[InBoneIndices.w]) * InWeights.w;

    float3 OutTangent = mul(InTangent, (float3x3)boneCB.boneTransforms[InBoneIndices.x]) * InWeights.x;
    OutTangent += mul(InTangent, (float3x3)boneCB.boneTransforms[InBoneIndices.y]) * InWeights.y;
    OutTangent += mul(InTangent, (float3x3)boneCB.boneTransforms[InBoneIndices.z]) * InWeights.z;
    OutTangent += mul(InTangent, (float3x3)boneCB.boneTransforms[InBoneIndices.w]) * InWeights.w;
    // Ends vertex skinning.

    Output[dispatchThreadID.x].position = OutPos;
    // Blending can output vertex normals of different length, which will skew interpolation if not normalized.
    Output[dispatchThreadID.x].normal   = normalize(OutNormal);
    //Output[DTid.x].normal   = OutNormal;
    Output[dispatchThreadID.x].texCoord = InTexCoord;
    Output[dispatchThreadID.x].tangent  = OutTangent;
}
