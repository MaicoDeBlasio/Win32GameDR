//--------------------------------------------------------------------------------------
// FullScreenQuadVS.hlsl
//
// Simple vertex shader to draw a full-screen quad
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#define HLSL
#include "RaytracingHlslCompat.h"
#include "Common.hlsli"

/*

    We use the 'big triangle' optimization that only requires three vertices to completely
    cover the full screen area.

    v0    v1        ID    NDC     UV
    *____*          --  -------  ----
    | | /           0   (-1,+1)  (0,0)
    |_|/            1   (+3,+1)  (2,0)
    | /             2   (-1,-3)  (0,2)
    |/
    *
    v2

*/

TexCoordVertexOutput main(uint vertexID : SV_VertexID)
{
    TexCoordVertexOutput vout;

    vout.texCoord = float2((vertexID << 1) & 2, vertexID & 2);

    // Projected tex coords to get linear depth in pixel shader have been moved to the vertex shader
    // since computation is identical for homogenous coords.
    // See Luna p.687
    float x =  vout.texCoord.x * 2 - 1;
    float y = -vout.texCoord.y * 2 + 1;

    //vout.projCoord = float2(x, y);

    // Procedurally generate each NDC vertex.
    // The big triangle produces a quad covering the screen in NDC space.
    vout.posH = float4(x, y, 0, 1);
    //vout.posH = float4(vout.projCoord, 0.f, 1.f);

    //vout.posH = float4(vout.texCoord.x * 2.f - 1.f, -vout.texCoord.y * 2.f + 1.f, 0.f, 1.f);
    //vout.posH = float4(vout.texCoord * float2(2.f,-2.f) + float2(-1.f,1.f), 0.f, 1.f);

    // Transform quad corners to view space near plane.
    float4 ph = mul(vout.posH, frameCB.invProj);
    vout.posV = ph.xyz / ph.w;

    return vout;
}

//[RootSignature(FullScreenQuadRS)]
//Interpolators main(uint vI : SV_VertexId)
//{
//    return main11(vI);
//}