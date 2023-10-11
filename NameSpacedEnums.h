#pragma once

namespace Globals
{
    const DXGI_FORMAT BackbufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    //const DXGI_FORMAT BackbufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;// DXGI_FORMAT_UNKNOWN;
    const DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    const float       RotationGain = DirectX::XM_PI / 1000.f; // radians
    const float       MovementGain = 5.f;

    //const uint32_t    FrameCount = 2;

    // Transforms NDC space [-1,+1]^2 to texture space [0,1]^2
    const Matrix      T = Matrix(
        0.5f, 0.f, 0.f, 0.f,
        0.f, -0.5f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.5f, 0.5f, 0.f, 1.f
    );
}

namespace DescriptorHeaps
{
    enum
    {
        SrvUav, Rtv,
        Count
    };
}

namespace RTVs
{
    enum
    {
        HdrRtv, Pass1Rtv, Pass2Rtv, BloomRtv, TonemapRtv,
        Count
    };
}

namespace RootSignatures
{
    enum
    {
        Graphics,// ComputeBlur, ComputeSkinning,
        Compute, // This compute root signature is used for all dispatches and the global DXR root signature.
        //DxrGlobal,
        //DxrLocal,
        Count
    };
}

//namespace GlobalRootSigParams
//{
//    enum
//    {
//        //OutputViewSlot = 0,       // u0
//        AccelerationStructure,// t0
//        //VertexBufferSlot,         // t1-t5
//        CameraConstants,      // b0
//        Count
//    };
//}

//namespace LocalRootSigParams {
//    enum {
//        MaterialConstantSlot = 0, // b1
//        //ViewportConstantSlot = 0,
//        Count
//    };
//}

// Root parameter slots for the graphics root signatures.
namespace GraphicsRootSigParams
{
    enum
    {
        FrameCB, MeshCB, //BoneCB,
        //ObjectDescriptorTableSlot,
        Count
    };
}

namespace ComputeRootSigParams
{
    enum
    {
        FrameCB, BoneCB,
        //AccelerationStructure,
        IndirectSrvTable,
        Count
    };
}

namespace PSOs
{
    enum
    {
        Cubes, MeshOpaque, MeshAlphaBlend,
        GeoSphere, AOBlurHorz, AOBlurVert, ShadowBlurHorz, ShadowBlurVert,
        Skinning, PostProcess, Fxaa,
        Count
    };
}

namespace RenderTextures
{
    enum
    {
        Hdr, Pass1, Pass2, Bloom, Tonemap,
        Count
    };
}

namespace PostProcessType
{
    enum
    {
        BloomExtract, BloomBlur, Blur, Copy,
        Count
    };
}

namespace TonemapType
{
    enum
    {
        Default, HDR10, Linear,
        Count
    };
}

// Procedural geometries built within the application.
namespace ProcGeometries
{
    enum
    {
        Cube, //Plane,
        GeoSphere, // We need a separate copy of the geometry for indirect drawing.
        Count
    };
}

namespace SDKMESHModels
{
    enum
    {
        Suzanne, Racetrack, Palmtree, MiniRacecar,
        Count
    };
}

namespace FBXModels
{
    enum
    {
        Dove,
        Count
    };
}

//namespace StructBuffers
//{
//    enum
//    {
//        //GeometryData,
//        InstanceData,
//        Count
//    };
//}

namespace StateObjects
{
    enum
    {
        AOPipeline, ShadowPipeline, MainPipeline,
        HitGroupCollection, // Also equal to pipeline state object count.
        Count
    };
}

namespace MeshType
{
    enum
    {
        Cube, Opaque, Transparent,
        Count
    };
}

namespace MeshGeometries
{
    enum
    {
        Cube, //Plane,
        Suzanne,
        RacetrackRoad, RacetrackSkirt, RacetrackMap,
        PalmtreeTrunk, PalmtreeCanopy,
        MiniRacecar,
        Dove, // FBX model.
        Count
    };
}

namespace BLASType
{
    enum
    {
        Static, Dynamic, Count
    };
}
