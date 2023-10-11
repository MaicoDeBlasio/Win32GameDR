//***************************************************************************************
// Forward rendering for skinned models, based on DefaultPS.hlsl by Frank Luna (C) 2015
//***************************************************************************************

#include "Common.hlsli"

VertexOut VS(SkinnedVertexIn vin)
{
    VertexOut vout = (VertexOut)0.f;

    // Perform vertex skinning.
    // Ignore BoneWeights.w and instead calculate the last weight value
    // to ensure all bone weights sum to unity.

    float4 weights = vin.boneWeights;
    //weights.w = 1.f - dot(weights.xyz, float3(1.f, 1.f, 1.f));

    weights.w = 1.f - (weights.x + weights.y + weights.z);

    float3 posL = mul(float4(vin.posL, 1.f), (float4x3)BoneTransforms[vin.boneIndices.x]).xyz * weights.x;
    posL += mul(float4(vin.posL, 1.f), (float4x3)BoneTransforms[vin.boneIndices.y]).xyz * weights.y;
    posL += mul(float4(vin.posL, 1.f), (float4x3)BoneTransforms[vin.boneIndices.z]).xyz * weights.z;
    posL += mul(float4(vin.posL, 1.f), (float4x3)BoneTransforms[vin.boneIndices.w]).xyz * weights.w;

    float3 normalL = mul(vin.normalL, (float3x3)BoneTransforms[vin.boneIndices.x]) * weights.x;
    normalL += mul(vin.normalL, (float3x3)BoneTransforms[vin.boneIndices.y]) * weights.y;
    normalL += mul(vin.normalL, (float3x3)BoneTransforms[vin.boneIndices.z]) * weights.z;
    normalL += mul(vin.normalL, (float3x3)BoneTransforms[vin.boneIndices.w]) * weights.w;

    float3 tangentL = mul(vin.tangentU, (float3x3)BoneTransforms[vin.boneIndices.x]) * weights.x;
    tangentL += mul(vin.tangentU, (float3x3)BoneTransforms[vin.boneIndices.y]) * weights.y;
    tangentL += mul(vin.tangentU, (float3x3)BoneTransforms[vin.boneIndices.z]) * weights.z;
    tangentL += mul(vin.tangentU, (float3x3)BoneTransforms[vin.boneIndices.w]) * weights.w;

    //float3 posL = float3(0.f, 0.f, 0.f);
    //float3 normalL = float3(0.f, 0.f, 0.f);
    //float3 tangentL = float3(0.f, 0.f, 0.f);
    //
    //[unroll]
    //for (uint i = 0u; i < 4u; ++i)
    //{
    //    // Assume no nonuniform scaling when transforming normals, so 
    //    // that we do not have to use the inverse-transpose.
    //
    //    posL += weights[i] * mul(float4(vin.posL, 1.f), gBoneTransforms[vin.boneIndices[i]]).xyz;
    //    normalL += weights[i] * mul(vin.normalL, (float3x3)gBoneTransforms[vin.boneIndices[i]]);
    //    tangentL += weights[i] * mul(vin.tangentU, (float3x3)gBoneTransforms[vin.boneIndices[i]]);
    //}

    // End vertex skinning

    // transform to world space
    float4 posW = mul(float4(posL, 1.f), World);
    vout.posW = posW.xyz;

    // assumes nonuniform scaling, otherwise needs inverse-transpose of world matrix
    vout.normalW = mul(normalL, (float3x3)World);
    vout.tangentW = mul(tangentL, (float3x3)World);

    // transform to homogenous clip space
    vout.posH = mul(posW, ViewProj);

    // pass texcoords to pixel shader
    vout.texCoord = vin.texCoord;

    //float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    //vout.TexC = mul(texC, gMatTransform).xy;

    // generate projective tex-coords to project shadow map and ssao map onto scene
    vout.shadowPosH = mul(posW, ShadowTransform);
    vout.ssaoPosH = mul(posW, ViewProjTex);

    return vout;
}
