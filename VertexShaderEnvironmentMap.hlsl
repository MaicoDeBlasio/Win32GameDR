//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//=============================================================================

// Modified to omit world matrix.
// Uses a view-projection matrix with a zeroed translation row-vector.

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"

//ConstantBuffer<FrameConstants> frameCB : register(b0);

VertexEnvMapOutput main(float4 posL : SV_POSITION)
{
    VertexEnvMapOutput vout;

    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.posH = mul(float4(posL.xyz, 0), frameCB.viewProj); // set position.w to zero out the translation
    //vout.posH = mul(posL, SkyViewProj);// .xyww;
    vout.posH.z = 0; // Draw on far plane using a reverse-z depth buffer
    //vout.posH.z = vout.posH.w; // Draw on far plane

    vout.texCoord = posL.xyz;  // Use local vertex position as cubemap lookup vector
    //vout.texCoord.z *= -1; // negate z for RH coord system

    return vout;
}

//SkyVertexOut VS(SkyVertexIn vin)
//{
//	SkyVertexOut vout = (SkyVertexOut)0.f;
//
//	// Use local vertex position as cubemap lookup vector.
//	vout.PosL = vin.PosL;
//	
//	// Transform to world space.
//	float4 posW = float4(vin.PosL, 1.f);
//	//float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
//
//	// Always center sky about camera.
//	posW.xyz += SkyEyePosW;
//
//	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
//	vout.PosH = mul(posW, SkyViewProj).xyww;
//	
//	return vout;
//}
