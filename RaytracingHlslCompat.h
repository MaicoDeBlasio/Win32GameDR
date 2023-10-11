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
//#ifndef RAYTRACINGHLSLCOMPAT_H
//#define RAYTRACINGHLSLCOMPAT_H

//**********************************************************************************************
//
// RaytracingHLSLCompat.h
//
// A header with shared definitions for C++ and HLSL source files. 
//
//**********************************************************************************************

#ifdef HLSL
#include "HlslCompat.hlsli"
#else
using namespace DirectX::SimpleMath;
using namespace DirectX;

// Shader will use byte encoding to access indices.
//typedef UINT16 Index;
#endif

// Set max recursion depth as low as needed, as drivers may apply optimization strategies for low recursion depths.
#define MAX_RAY_RECURSION_DEPTH 2   // ~ Primary rays + reflections + shadow rays from reflected geometry.
#define MAX_BONES 50                // Skinned animation bone palette element limit for Fbx models.
//#define CUBE_INSTANCE_COUNT 3       // added this

namespace SrvUAVs
{
    enum
    {
        TLASBufferSrv,      // Scene acceleration structures.
        DxrUav, DxrSrv, DxrPrevSrv,    // DXR texture views.
        HdrSrv, Pass1Srv, Pass2Srv, BloomSrv, TonemapSrv, // Render textures.
        /*GeometryBufferSrv,*/ InstanceBufferSrv, // Scene instance structured buffers.
        // Occlusion manager classes require contiguous descriptors in this order.
        AmbientBuffer0Uav, AmbientBuffer1Uav, AmbientBuffer0Srv, NormalDepthUav, NormalDepthSrv, AmbientBufferPrevSrv,
        // AmbientMap0Srv, AmbientMap0Uav, AmbientMap1Srv, AmbientMap1Uav, NormalMapSrv, DepthMapSrv, RandomVectorMapSrv,
        ShadowBuffer0Uav,  ShadowBuffer1Uav,  ShadowBuffer0Srv, ShadowBufferPrevSrv,
        // Each object requires contiguous descriptors in this order (should not be necessary with dynamic heap indexing).
        CubeVertexBufferSrv, CubeIndexBufferSrv, CubeAlbedoSrv, CubeNormalSrv, CubeEmissiveSrv, CubeRMASrv,
        //PlaneVertexBufferSrv, PlaneIndexBufferSrv, TreeAlbedoSrv,
        SuzanneVertexBufferSrv, SuzanneIndexBufferSrv, SuzanneAlbedoSrv, SuzanneNormalSrv,
        RacetrackRoadVertexBufferSrv, RacetrackRoadIndexBufferSrv, RacetrackRoadAlbedoSrv, RacetrackRoadNormalSrv, RacetrackRoadRMASrv,
        RacetrackSkirtVertexBufferSrv, RacetrackSkirtIndexBufferSrv, RacetrackSkirtAlbedoSrv, RacetrackSkirtNormalSrv, RacetrackSkirtRMASrv,
        RacetrackMapVertexBufferSrv, RacetrackMapIndexBufferSrv, RacetrackMapAlbedoSrv, // Racetrack map uses default normal and RMA textures.
        PalmtreeTrunkVertexBufferSrv, PalmtreeTrunkIndexBufferSrv, PalmtreeAlbedoSrv, PalmtreeNormalSrv, PalmtreeRMASrv,
        PalmtreeCanopyVertexBufferSrv, PalmtreeCanopyIndexBufferSrv, // Palm tree canopy shares textures with palm tree trunk.
        MiniRacecarVertexBufferSrv, MiniRacecarIndexBufferSrv, MiniRacecarAlbedoSrv, MiniRacecarRMASrv, // Mini racecar uses default normal texture.
        DoveVertexBufferSrv, DoveSkinnedVertexBufferUav, DoveIndexBufferSrv, DoveAlbedoSrv, DoveNormalSrv,
        // PBR textures.
        //DiffuseIBLSrv, SpecularIBLSrv,
        EnvironmentMapSrv, // Miss shader cubemaps
        DefaultNormalSrv, DefaultRMASrv, // Default textures
        // Indirect drawing buffers per backbuffer.
        FrameConstantsSrv_0, CommandBufferSrv_0,
        FrameConstantsSrv_1, CommandBufferSrv_1,
        // CPU writeable structured buffers to store previous frame world transforms.
        PrevFrameDataBufferSrv_0, PrevFrameDataBufferSrv_1,
        Count
    };
}

// This sample uses one geometry type (triangle).
// Mixing of geometry types within a BLAS is not supported.
//namespace GeometryType
//{
//    enum
//    {
//        Triangle,
//        //AABB,       // Procedural geometry with an application provided AABB.
//        Count
//    };
//}

// Ray types traced in this sample.
namespace RayType
{
    enum
    {
        Radiance,  // ~ Primary, reflected camera/view rays calculating color for each hit.
        Occlusion, // ~ Shadow/visibility/ao rays, only testing for occlusion.
        Count
    };
}

///// Constant buffer structs must be 16-byte aligned /////

struct BlurConstants
{
    // We cannot have an array entry in a constant buffer that gets mapped onto root constants, so list each element.
    Vector4  blurWeights[3];
    uint32_t blurMap0UavID;
    uint32_t blurMap1UavID;
    uint32_t normalDepthMapSrvID;
};
struct FrameConstants
{
    // Current frame camera matrices.
    Matrix   viewProj;
    Matrix   invProj;
    Matrix   invViewProj;
    Matrix   viewProjTex;   // Required for RTAO map projection in pixel shaders.
    // Previous frame camera matrices.
    Matrix   prevViewProjTex;
    //Matrix   prevInvProj;
    //Matrix   prevInvViewProj;

    Vector4  cameraPos;
    Vector4  lightDiffuse;
    Vector4  lightAmbient;
    Vector3  lightDir;
    // Input to random number seed for rtao.
    uint32_t frameCount;
    //BackbufferSize backbufferSize;
    //Vector2  projValues;
    //Vector4 offsetVectors[14];
    uint32_t tlasBufferSrvID;
    uint32_t instBufferSrvID;
    uint32_t prevFrameBufferSrvID;   // Stores previous frame world transforms.
    uint32_t isFirstFrame;
};
struct MeshConstants
{
    Matrix   world;
    uint32_t instanceID;
};
struct BoneConstants
{
    XMFLOAT4X3 boneTransforms[MAX_BONES];
};

// Structured buffer element structures.
struct Material
{
    // Constants for untextured materials.
    Vector4  albedo;     // albedo rgb value
    Vector4  emissive;   // emissive rgb value
    Vector4  RMA;        // r: ambient occlusion; g: roughness; b: metalness

    // Texture indices.
    uint32_t albedoTexIndex;
    uint32_t normalTexIndex;
    uint32_t emissiveTexIndex;
    uint32_t RMATexIndex;
    float    textureWrapFactor;
    //uint32_t diffuseIBLTexIndex;
    //uint32_t specularIBLTexIndex;

    //Vector3 diffuseColor;
    //float   unused;
    //float   reflectanceCoef;
    //float   diffuseCoef;
    //float   specularCoef;
    //float   specularPower;
};

// To substitute for HeapIndexConstants
struct MeshGeometryData
{
    uint32_t vertexBufferIndex; // Offset of vertex buffer descriptor into heap.
    uint32_t indexBufferIndex;  // Offset of index buffer descriptor into heap.
    uint32_t isIndex16;         // bool datatype does not produce correct boolean logic in HLSL.
    //bool     indexSize16;
};

struct InstanceData
{
    MeshGeometryData geometryData;
    //uint32_t vertexBufferIndex;
    //uint32_t indexBufferIndex;
    //uint32_t indexSize16;
    //uint32_t diffuseTextureIndex;
    //uint32_t normalTextureIndex;
    Material material;
    //Vector3 albedo;
    //Vector4 albedo; // Uses Vector4 to satisfy 16 byte alignment
    //float reflectanceCoef;
    //float diffuseCoef;
    //float specularCoef;
    //float specularPower;
};

struct PrevFrameData
{
    Matrix world;
};

// cbuffer struct for ao raytracing pass
//struct HeapIndexConstants
//{
//    uint32_t vbIndex; // offset of vertex buffer descriptor into heap
//};

//struct SsaoConstants
//{
//    Matrix Proj;
//    Matrix InvProj;
//    Matrix ProjTex;
//    Vector4 OffsetVectors[14];
//
//    // For SsaoBlur.hlsl
//    Vector4 BlurWeights[3];
//    Vector2 InvRenderTargetSize;// = { 0, 0 };
//
//    // Coordinates given in view space.
//    float OcclusionRadius;// = 0.5f;
//    float OcclusionFadeStart;// = 0.2f;
//    float OcclusionFadeEnd;// = 2.f;
//    float SurfaceEpsilon;// = 0.05f;
//};

//struct AoBlurDirection
//{
//    bool HorizontalBlur;
//};

/*
// We use this struct to store helper rays to calculate ray differentials.
// We cannot use the 'Ray' struct definition here since it clashes with Ray struct defined in DirectX::SimpleMath.
struct AuxilliaryRay
{
    Vector3 origin;
    Vector3 direction;
};
// Hit information, aka ray payload.
// Note that the payload should be kept as small as possible, and that its size must be declared in the corresponding
// We cannot use a Ray struct in this payload because definition for Ray already exists in DirectX::SimpleMath.
struct AOPayload
{
    Vector4 normalAndDepth;
    float occlusion;
    //float    ambientAccess;
    //uint32_t occlusion;     // Rtao accumulator sum.
    //uint32_t recursionDepth;
    AuxilliaryRay ddxRay;
    AuxilliaryRay ddyRay;
};

struct ShadowPayload
{
    float occlusion;
    //bool isShadowRayMiss;
    AuxilliaryRay ddxRay;
    AuxilliaryRay ddyRay;
};

struct ColorPayload
{
    Vector3 color;
    uint32_t recursionDepth;
    //bool isShadowRayMiss;    // Shadow ray flag.
    //bool isShadowRayHit;
    AuxilliaryRay ddxRay;
    AuxilliaryRay ddyRay;
};
//*/
// Structured buffer vertex element structs.
struct VertexCube
{
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
};

//struct VertexPlane
//{
//    Vector3 position;
//    Vector3 normal;
//    Vector2 texCoord;
//};

struct VertexPosNormalTexTangent
{
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    Vector3 tangent;
    //Vector3 bitangent;	// Unused but required for correct stride in dxr shaders.
};

struct VertexFbxBones
{
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    Vector3 tangent;
    Vector4 boneWeights;
    uint32_t boneIndices;
};

//struct VertexSkinnedFbx
//{
//    Vector3 position;
//    Vector3 normal;
//    Vector2 texCoord;
//    Vector3 tangent;
//};

// From: http://blog.selfshadow.com/publications/s2015-shading-course/hoffman/s2015_pbs_physics_math_slides.pdf
static const Vector4 ChromiumReflectance = Vector4(0.549f, 0.556f, 0.554f, 1.0f);
static const Vector4 BackgroundColor = Vector4(0.8f, 0.9f, 1.0f, 1.0f);

//struct RayGenViewport
//{
//    float left;
//    float top;
//    float right;
//    float bottom;
//};
//
//struct RayGenConstantBuffer
//{
//    RayGenViewport viewport;
//    RayGenViewport stencil;
//};

//#endif // RAYTRACINGHLSLCOMPAT_H
