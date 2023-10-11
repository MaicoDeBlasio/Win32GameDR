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

#pragma once

static const float PI = 3.14159265f;

// Register slots
//RaytracingAccelerationStructure Scene : register(t0);
//RWTexture2D<float4> gOutput : register(u0);
//StructuredBuffer<Vertex> Vertices : register(t1);
//ByteAddressBuffer Indices : register(t2);
//StructuredBuffer<VertexPlane> PlaneVertices : register(t3);
//ByteAddressBuffer PlaneIndices : register(t4);
//Texture2D PlaneDiffuseMap : register(t5);

ConstantBuffer<FrameConstants> frameCB: register(b0);
ConstantBuffer<MeshConstants>  meshCB : register(b1);
//ConstantBuffer<BoneConstants>  boneCB : register(b2);
//ConstantBuffer<HeapIndexConstants> heapIndexCB : register(b1);

// Sampler state registers must match declaration in C++.
SamplerState AnisoClamp  : register(s0);
SamplerState LinearClamp : register(s1);
SamplerState PointClamp  : register(s2);
SamplerState AnisoWrap   : register(s3);
SamplerState LinearWrap  : register(s4);

// Typedefs and structures.

/*
A DXR intrinsic struct that captures attributes of the triangle
primitive being intersected by the currently traced ray.
struct BuiltInTriangleIntersectionAttributes
{
    float2 barycentrics.
}
*/

typedef BuiltInTriangleIntersectionAttributes Attributes;

struct Ray
{
    float3 origin;
    float3 direction;
};

/*
// Hit information, aka ray payload.
// Note that the payload should be kept as small as possible, and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobject (Edit: raypayload field is ignored when PAQ are enabled in SM 6.7 or higher).
// We cannot use a Ray struct in this payload because definition for Ray already exists in DirectX::SimpleMath.
struct [raypayload] AOPayload
{
    float4 normalAndDepth : read(caller) : write(closesthit,miss);
    float occlusion : read(caller) : write(closesthit,miss);
    Ray ddxRay : read(anyhit) : write(caller);
    Ray ddyRay : read(anyhit) : write(caller);
};

struct [raypayload] ShadowPayload
{
    float occlusion : read(caller) : write(closesthit,miss);
    Ray ddxRay : read(anyhit) : write(caller);
    Ray ddyRay : read(anyhit) : write(caller);
};

struct [raypayload] ColorPayload
{
    float3 color : read(caller) : write(closesthit,miss);
    uint recursionDepth : read(caller) : write(closesthit,miss);
    Ray ddxRay : read(anyhit) : write(caller);
    Ray ddyRay : read(anyhit) : write(caller);
};
//*/

///*
struct AOPayload
{
    float4 normalAndDepth;
    float occlusion;
    Ray ddxRay;
    Ray ddyRay;
};

struct ShadowPayload
{
    float occlusion;
    Ray ddxRay;
    Ray ddyRay;
};

struct ColorPayload
{
    float3 color;
    uint recursionDepth;
    Ray ddxRay;
    Ray ddyRay;
};
//*/

struct PartialDerivatives
{
    float2 ddxUV;
    float2 ddyUV;
};

// Vertex shader structs for rasterization pipeline.
struct VertexCubeInput
{
    float3 posL		: SV_POSITION;
    float3 normalL	: NORMAL;
    float2 texCoord : TEXCOORD;
    float4x3 world  : INSTMATRIX;
    //float4 instColor : INSTCOLOR;
};
//struct VertexPlaneInput
//{
//    float3 posL    : SV_POSITION;
//    float3 normalL : NORMAL;
//    float2 texCoord: TEXCOORD;
//};
struct VertexMeshInput
{
    float3 posL    : SV_POSITION;
    float3 normalL : NORMAL;
    float2 texCoord: TEXCOORD;
    float3 tangentL: TANGENT;
    //float3 binormalL : BINORMAL;
};
struct VertexSkinnedInput
{
    float3 posL       : SV_POSITION;
    float3 normalL    : NORMAL;
    float2 texCoord   : TEXCOORD;
    float3 tangentL   : TANGENT;
    float4 boneWeights: BONEWEIGHT;
    uint4 boneIndices : BONEINDEX;
};
struct VertexOutput
{
    float4 posH     : SV_POSITION;
    float4 scrnPosH : POSITION0;    // Screen space position.
    float3 posW     : POSITION1;
    float3 normalW  : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangentW : TANGENT;
    //float4 color   : COLOR;
    //float3 color   : COLOR;
    uint instanceID : SV_InstanceID; // Must pass to pixel shader from vertex shader.
    //nointerpolation uint instance_id : INSTANCE;
};
struct VertexEnvMapOutput
{
    // texCoord will be the only input parameter to the pixel shader signature,
    // so it's placed first in the struct to allow this.
    float3 texCoord: TEXCOORD;
    float4 posH    : SV_POSITION;
};
struct TexCoordVertexOutput
{
    // Where texcoord is the only parameter that needs to be passed to
    // a pixel shader input signature, we place it first in the struct to allow this.
    float2 texCoord : TEXCOORD;
    float4 posH : SV_POSITION;
    float3 posV : POSITION;
    //float2 projCoord : TEXCOORD1;
};

// Functions

// Source: https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli
float3 RemoveSRGBCurve(in float3 x)
{
    // Approximates pow(x, 2.2f).
    // We are now using HLSL 2021 logical operator short circuiting for scalar types.
    return select(x < 0.04045f, x / 12.92f, pow((x + 0.055f) / 1.055f, 2.4f));
    //return x < 0.04045f ? x / 12.92f : pow((x + 0.055f) / 1.055f, 2.4f);
}

float3 ApplySRGBCurve(in float3 x)
{
    // Approximates pow(x, 1.f/ 2.2f)
    return select(x < 0.0031308f, 12.92f * x, 1.055f * pow(x, 1.f / 2.4f) - 0.55f);
    //return x < 0.0031308f ? 12.92f * x : 1.055f * pow(x, 1.f / 2.4f) - 0.55f;
}

// Load three 16 bit indices from a byte addressed buffer.
//uint3 Load3x16BitIndices(uint offsetBytes)
uint3 Load3x16BitIndices(in uint offsetBytes, ByteAddressBuffer indexBuffer)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint  dwordAlignedOffset = offsetBytes & ~3;
    //const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);
    const uint2 four16BitIndices = indexBuffer.Load2(dwordAlignedOffset);

    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x =  four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z =  four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y =  four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

uint3 LoadIndices(in uint isIndex16, in uint primitiveIndex, ByteAddressBuffer indexBuffer)
{
    // Get the base index of the triangle's first 16 or 32 bit index.
    const uint indexSizeInBytes    = isIndex16 ? 2 : 4;
    const uint triangleIndexStride = indexSizeInBytes * 3;
    const uint baseIndex = primitiveIndex * triangleIndexStride;

    // Load three 16 or 32 bit indices for the triangle, depending on index size.
    return isIndex16 ? Load3x16BitIndices(baseIndex, indexBuffer) : indexBuffer.Load3(baseIndex);
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
    // DXR instrinsic HLSL functions.
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
template <typename T>
T HitAttribute(in T vertexAttribute[3], in Attributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

/*
// Retrieve float2 attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float2 HitAttribute(in float2 vertexAttribute[3], in Attributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Retrieve float3 attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(in float3 vertexAttribute[3], in Attributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}
*/

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
//inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
Ray GenerateCameraRay(in uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    const float2 xy  = index + 0.5f; // Center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2 - 1;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 worldPos = mul(float4(screenPos, 0, 1), projectionToWorld); // i.e. camera to world space.
    worldPos.xyz /= worldPos.w;

    Ray ray;
    ray.origin    = cameraPosition;
    ray.direction = normalize(worldPos.xyz - ray.origin);

    return ray;
}

// This version passes in the pixel location from a pixel shader.
Ray GenerateCameraRay(in float2 screenPos, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    // Unproject the pixel coordinate into a ray.
    float4 worldPos = mul(float4(screenPos, 0, 1), projectionToWorld); // i.e. camera to world space.
    worldPos.xyz /= worldPos.w;

    Ray ray;
    ray.origin    = cameraPosition;
    ray.direction = normalize(worldPos.xyz - ray.origin);

    return ray;
}

// See Zeng et al, "Temporally Reliable Motion Vectors for Better Use of Temporal Information",
// Ray Tracing Gems II, Chapter 25, pp. 401-416
float2 CalculatePrevFrameTexCoords(in float3 posW, in float4x4 prevWorld, in float4x4 prevViewProjTex)
{
    // Transform world space hit position to object (local) space.
    const float3 posL = mul(float4(posW, 1), WorldToObject4x3());
    
    // Transform posL to previous frame's world position.
    const float4 prevPosW     = mul(float4(posL, 1), prevWorld);
    const float4 prevScrnPosH = mul(prevPosW, prevViewProjTex);
    //const float4 prevScrnPosH = mul(float4(prevPosW, 1), prevViewProjTex);
    
    // Complete the uv projection.
    const float4 prevScrnPos = prevScrnPosH / prevScrnPosH.w;
    return prevScrnPos.xy;
}

float3 RayPlaneIntersection(in float3 planeOrigin, in float3 planeNormal, in Ray ray)
{
    const float t = dot(-planeNormal, ray.origin - planeOrigin) / dot(planeNormal, ray.direction);
    return ray.origin + ray.direction * t;
}

// REF: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
// From "Real-Time Collision Detection" by Christer Ericson
float3 BarycentricCoordinates(in float3 pt, in float3 v0, in float3 v1, in float3 v2)
{
    const float3 e0 = v1 - v0;
    const float3 e1 = v2 - v0;
    const float3 e2 = pt - v0;
    const float d00 = dot(e0, e0);
    const float d01 = dot(e0, e1);
    const float d11 = dot(e1, e1);
    const float d20 = dot(e2, e0);
    const float d21 = dot(e2, e1);
    const float denom = 1.f / (d00 * d11 - d01 * d01);
    const float v = (d11 * d20 - d01 * d21) * denom;
    const float w = (d00 * d21 - d01 * d20) * denom;
    const float u = 1.f - v - w;
    return float3(u, v, w);
}

PartialDerivatives GetPartialDerivatives(in float3 xOffset, in float3 yOffset, in float3 vertexPos[3], in float2 vertexTexC[3], in float2 uv)
{
    PartialDerivatives pd;
    // Compute barycentrics 
    const float3 baryX = BarycentricCoordinates(xOffset, vertexPos[0], vertexPos[1], vertexPos[2]);
    const float3 baryY = BarycentricCoordinates(yOffset, vertexPos[0], vertexPos[1], vertexPos[2]);

    // Compute UVs and take the difference
    const float3x2 uvMatrix = float3x2(vertexTexC);

    pd.ddxUV = mul(baryX, uvMatrix) - uv;
    pd.ddyUV = mul(baryY, uvMatrix) - uv;
    return pd;
}

//static const float InShadowRadiance = 0.25f;

// Diffuse lighting calculation.
float CalculateDiffuseCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal)
{
    return saturate(dot(-incidentLightRay, normal));
    //float fNDotL = saturate(dot(-incidentLightRay, normal));
    //return fNDotL;
}

// Phong lighting specular component
float CalculateSpecularCoefficient(in float3 hitPosition, in float3 incidentLightRay, in float3 normal, in float specularPower)
{
    const float3 reflectedLightRay = normalize(reflect(incidentLightRay, normal));
    return pow(saturate(dot(reflectedLightRay, normalize(-WorldRayDirection()))), specularPower);
}

// Phong lighting model = ambient + diffuse + specular components.
// Modified for directional light.
float3 CalculatePhongLighting(in float3 albedo, in float3 normal, in bool isInShadow,
    in float3 lightDirection, in float3 lightDiffuseColor, in float3 lightAmbientColor, // added these input params
    //in float3 lightPosition, in float4 lightDiffuseColor, in float4 lightAmbientColor,
    in float diffuseCoef = 1, in float specularCoef = 1, in float specularPower = 50)
{
    const float3 hitPosition = HitWorldPosition();
    //float3 lightPosition = g_sceneCB.lightPosition.xyz;
    const float shadowFactor = isInShadow ? 0 : 1;
    //const float shadowFactor = isInShadow ? InShadowRadiance : 1;

    const float3 incidentLightRay = lightDirection;
    //float3 incidentLightRay = normalize(hitPosition - lightPosition);

    // Diffuse component.
    //float4 lightDiffuseColor = g_sceneCB.lightDiffuseColor;
    const float Kd = CalculateDiffuseCoefficient(hitPosition, incidentLightRay, normal);
    const float3 diffuseColor = shadowFactor * diffuseCoef * Kd * lightDiffuseColor * albedo;

    // Specular component.
    float3 specularColor = 0;
    if (!isInShadow)
    {
        const float3 lightSpecularColor = 1;
        const float Ks = CalculateSpecularCoefficient(hitPosition, incidentLightRay, normal, specularPower);
        specularColor = specularCoef * Ks * lightSpecularColor;
    }

    // Ambient component.
    // Fake AO: Darken faces with normal facing downwards/away from the sky a little bit.
    // Edit: We are using ambient access data obtained from rtao, so do not implement fake ao here!

    //float4 ambientColor = g_sceneCB.lightAmbientColor;
    //float3 ambientColorMin = lightAmbientColor - 0.1;
    //float3 ambientColorMax = lightAmbientColor;
    //float4 ambientColorMin = g_sceneCB.lightAmbientColor - 0.1;
    //float4 ambientColorMax = g_sceneCB.lightAmbientColor;
    //float a = 1 - saturate(dot(normal, float3(0, -1, 0)));

    const float3 ambientColor = albedo * lightAmbientColor;
    //float3 ambientColor = albedo * lerp(ambientColorMin, ambientColorMax, a);

    return ambientColor + diffuseColor + specularColor;
}

// Fresnel reflectance - schlick approximation.
float3 FresnelReflectanceSchlick(in float3 I, in float3 N, in float3 f0)
{
    const float cosi = saturate(dot(-I, N));
    return f0 + (1 - f0) * pow(1 - cosi, 5);
}

// Utility constants & functions for our ambient occlusion shader.
//static const float OcclusionRadius = 0.5f;
// Ray extent constants.
static const float OcclusionFadeStart  = 0.01f;
static const float OcclusionFadeEnd    = 0.1f;
static const float SurfaceEpsilon      = 0.001f; // Only used by inline ray tracing in pixel shader.
static const float MaxAO_PrimRayLength = 100;
static const float MaxPrimaryRayLength = 5000;

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint InitRand(in uint val0, in uint val1, in uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;

    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float NextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 GetPerpendicularVector(in float3 u)
{
    const float3 a = abs(u);
    const uint xm  = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    const uint ym  = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    const uint zm  = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 GetCosHemisphereSample(inout uint randSeed, in float3 hitNorm)
{
    // Get two random numbers to select our sample with.
    const float2 randVal = float2(NextRand(randSeed), NextRand(randSeed));

    // Cosine weighted hemisphere sample from RNG
    const float3 bitangent = GetPerpendicularVector(hitNorm);
    const float3 tangent   = cross(bitangent, hitNorm);
    const float r   = sqrt(randVal.x);
    const float phi = 2.f * PI * randVal.y;
    //const float phi = 2.f * 3.14159265f * randVal.y;

    // Get our cosine-weighted hemisphere lobe sample direction.
    return tangent * r * cos(phi) + bitangent * r * sin(phi) + hitNorm.xyz * sqrt(1.f - randVal.x);
    //return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

// Pharr et al, Physically Based Rendering, Section 13.6.4 Sampling a Cone, p.781
float3 UniformSampleCone(inout uint randSeed, in float3 rayDir, in float thetaMaxDeg)
{
    const float cosThetaMax = cos(thetaMaxDeg * PI / 180.f);

    // Get two uniform random floats between [0..1]
    const float2 u = float2(NextRand(randSeed), NextRand(randSeed));

    const float cosTheta = (1.f - u.x) + u.x * cosThetaMax;
    const float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    const float phi = u.y * 2.f * PI;

    // Calculate the random cartesian offsets (third term adjusted for right-handed coordinate system).
    const float3 rayDirOffset = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, 1.f - cosTheta);
    //const float3 rayDirOffset = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    return normalize(rayDir + rayDirOffset);
}

// Determines how much the sample point q occludes the point p as a function of distZ.
float OcclusionFunction(in float distZ)
{
    /*
       If depth(q) is "behind" depth(p), then q cannot occlude p.  Moreover, if
       depth(q) and depth(p) are sufficiently close, then we also assume q cannot
       occlude p because q needs to be in front of p by Epsilon to occlude p.

       We use the following function to determine the occlusion.


             1.0     -------------\
                     |           |  \
                     |           |    \
                     |           |      \
                     |           |        \
                     |           |          \
                     |           |            \
        ------|------|-----------|-------------|---------|--> zv
              0     Eps          z0            z1
    */

    float occlusion = 0;
    if (distZ > SurfaceEpsilon)
    {
        const float fadeLength = OcclusionFadeEnd - OcclusionFadeStart;

        // Linearly decrease occlusion from 1 to 0 as distZ goes 
        // from gOcclusionFadeStart to gOcclusionFadeEnd.	
        occlusion = saturate((OcclusionFadeEnd - distZ) / fadeLength);
        //occlusion = saturate((distZ - OcclusionFadeEnd) / fadeLength); // 0 to 1
    }

    return occlusion;
}

float2 GetTextureCoords()
{
    const uint2 threadID   = DispatchRaysIndex().xy;
    const uint2 dimensions = DispatchRaysDimensions().xy;

    const float u = (float)threadID.x / (float)dimensions.x;
    const float v = (float)threadID.y / (float)dimensions.y;

    return float2(u, v);
}

//---------------------------------------------------------------------------------------
// Transforms a normal map sample to world space.
// Edit: Modified to use a two-component normal map sample.
//---------------------------------------------------------------------------------------
float3 NormalSampleToWorldSpace(in float2 normalMapSample, in float3 unitNormalW, in float3 tangentW)
//float3 NormalSampleToWorldSpace(in float3 normalMapSample, in float3 unitNormalW, in float3 tangentW)
{
    // Uncompress each component from [0,1] to [-1,1].
    // Note: If normal maps are compressed to BC5_SNORM format, components will be in [-1,1] range and decompression not required.
    const float2 normalT2 = 2.f * normalMapSample - 1.f;
    //const float3 normalT = 2 * normalMapSample - 1;

    const float z = sqrt(1.f - dot(normalT2, normalT2));
    
    const float3 normalT = float3(normalT2.x, normalT2.y, z);

    // Build orthonormal basis.
    const float3 N = unitNormalW;
    const float3 T = tangentW - dot(tangentW, N) * N;
    const float3 B = cross(N, T);

    const float3x3 TBN = float3x3(T, B, N);

    // Transform from tangent space to world space.
    // Not returning a normalized vector will lead to shading anomalies on some geometries.
    return normalize(mul(normalT, TBN));
    //return mul(normalT, TBN);
}

// Johannes Hanika, "Hacking the Shadow Terminator", Ray Tracing Gems II, Chapter 4, pp. 65-76
float3 GetOffsetShadowRayOrigin(in float3 hitPos, in float3 vertexPos[3], in float3 vertexNormals[3], in Attributes attr)
{
    // Get distance vectors from triangle vertices.
    float3 tmpu = hitPos - vertexPos[0];
    float3 tmpv = hitPos - vertexPos[1];
    float3 tmpw = hitPos - vertexPos[2];
    
    // Project these onto the tangent planes defined by the shading normals.
    const float dotu = min(0, dot(tmpu, vertexNormals[0]));
    const float dotv = min(0, dot(tmpv, vertexNormals[1]));
    const float dotw = min(0, dot(tmpw, vertexNormals[2]));
    
    tmpu -= dotu * vertexNormals[0];
    tmpv -= dotv * vertexNormals[1];
    tmpw -= dotw * vertexNormals[2];
    
    // Finally P' is the barycentric mean of these three.
    const float v = attr.barycentrics.x;
    const float w = attr.barycentrics.y;
    const float u = 1 - v - w;
    
    return hitPos + u * tmpu + v * tmpv + w * tmpw;
}

// This version of the function can be called by non-DXR shaders.
float3 GetOffsetShadowRayOrigin(in float3 hitPos, in float3 vertexPos[3], in float3 vertexNormals[3])
{
    // Get distance vectors from triangle vertices.
    float3 tmpu = hitPos - vertexPos[0];
    float3 tmpv = hitPos - vertexPos[1];
    float3 tmpw = hitPos - vertexPos[2];
    
    // Project these onto the tangent planes defined by the shading normals.
    const float dotu = min(0, dot(tmpu, vertexNormals[0]));
    const float dotv = min(0, dot(tmpv, vertexNormals[1]));
    const float dotw = min(0, dot(tmpw, vertexNormals[2]));
    
    tmpu -= dotu * vertexNormals[0];
    tmpv -= dotv * vertexNormals[1];
    tmpw -= dotw * vertexNormals[2];
    
    // Finally P' is the barycentric mean of these three.
    const float3 bary = BarycentricCoordinates(hitPos, vertexPos[0], vertexPos[1], vertexPos[2]);
    
    return hitPos + bary.x * tmpu + bary.y * tmpv + bary.z * tmpw;
}

// Wachter & Binder, "A Fast and Robust Method for Avoiding Self-Intersection", Ray Tracing Gems, Chapter 6, pp.77-85
// Normal points outward for rays exiting the surface, else is flipped.
static const float Origin      = 1.f / 32.f;
static const float FloatScale = 1.f / 65536.f;
static const float IntScale   = 256.f;

float3 GetOffsetRayOrigin(in float3 p, in float3 n)
{
    const int3 of_i  = IntScale * n;
    
    const float3 p_i = float3(
        asfloat(asint(p.x) + (p.x < 0 ? -of_i.x : of_i.x)),
        asfloat(asint(p.y) + (p.y < 0 ? -of_i.y : of_i.y)),
        asfloat(asint(p.z) + (p.z < 0 ? -of_i.z : of_i.z)));
    
    return float3(
        abs(p.x) < Origin ? p.x + FloatScale * n.x : p_i.x,
        abs(p.y) < Origin ? p.y + FloatScale * n.y : p_i.y,
        abs(p.z) < Origin ? p.z + FloatScale * n.z : p_i.z);
}

// Conty Estevez, Lecocq & Stein, "A Microfacet-Based Shadowing Function to Solve the Bump Terminator Problem",
// Ray Tracing Gems, Chapter 12, pp.149-158
// Shadowing factor, which can be used as a multiplier for either the incoming light or the BSDF evaluation.
float BumpShadowingFactor(in float3 Ngeom, in float3 Nbump, in float3 Ld)
{
    const float cos_d  = min(abs(dot(Ngeom, Nbump)), 1);
    const float cos_d2 = cos_d * cos_d;
    const float tan2_d = (1.f - cos_d2) / cos_d2;
    const float alpha2 = saturate(0.125f * tan2_d); // alpha^2 parameter from normal divergence.
    //const float alpha2 = clamp(0.125f * tan2_d, 0, 1);

    const float cos_i  = max(abs(dot(Ngeom, Ld)), 1e-6f);
    const float cos_i2 = cos_i * cos_i;
    const float tan2_i = (1.f - cos_i2) / cos_i2;
    return 2.f / (1.f + sqrt(1.f + alpha2 * tan2_i));
}

/*
// Christian Schuler, "Normal Mapping without Precomputed Tangents", ShaderX 5, Chapter 2.6, pp. 131-140
// See also follow-up blog post: http://www.thetenthplanet.de/archives/1180
float3x3 CalculateTBN(float3 p, float3 n, float2 tex)
{
    // DerivCoarseX & DerivCoarseY opcodes are not valid in dxr shaders.
    float3 dp1 = ddx(p);
    float3 dp2 = ddy(p);
    float2 duv1 = ddx(tex);
    float2 duv2 = ddy(tex);

    float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
    float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
    float3 t = normalize(mul(float2(duv1.x, duv2.x), inverseM));
    float3 b = normalize(mul(float2(duv1.y, duv2.y), inverseM));
    return float3x3(t, b, n);
}

float3 PeturbNormal(float3 localNormal, float3 position, float3 normal, float2 texCoord)
{
    const float3x3 TBN = CalculateTBN(position, normal, texCoord);
    return normalize(mul(localNormal, TBN));
}

float3 TwoChannelNormalX2(float2 normal)
{
    float2 xy = 2.f * normal - 1.f;
    float z = sqrt(1 - dot(xy, xy));
    return float3(xy.x, xy.y, z);
}
*/
