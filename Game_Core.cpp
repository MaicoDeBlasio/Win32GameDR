//
// Game_Core.cpp
//

// Core methods for Game class:
//  Constructor/Destructor
//  Initialize
//  Tick
//  Clear
//  Message & resize handlers
//  CreateDevice/WindowSizeResources
//  ReleaseDevice/WindowSizeResources
//  BuildGeometry (includes materials & uploading resources to gpu)
//  OnDeviceLost
//  CalculateFrameStats
//  ApplyToneMapping
//  CopyRaytracingOutputToBackbuffer

#include "pch.h"
#include "Game.h"
#include "Shaders/ComputeShaderAOBlurHorz.hlsl.h"
#include "Shaders/ComputeShaderAOBlurVert.hlsl.h"
#include "Shaders/ComputeShaderFinalPostProcess.hlsl.h"
#include "Shaders/ComputeShaderShadowBlurHorz.hlsl.h"
#include "Shaders/ComputeShaderShadowBlurVert.hlsl.h"
#include "Shaders/ComputeShaderSkinning.hlsl.h"
#include "Shaders/PixelShaderCubes.hlsl.h"
#include "Shaders/PixelShaderEnvironmentMap.hlsl.h"
#include "Shaders/PixelShaderFxaa.hlsl.h"
#include "Shaders/PixelShaderMesh.hlsl.h"
#include "Shaders/RaytracingShaderAO.hlsl.h"
#include "Shaders/RaytracingShaderColor.hlsl.h"
#include "Shaders/RaytracingShaderShadows.hlsl.h"
//#include "Shaders/RaytracingShaderAO_PAQ.hlsl.h"
//#include "Shaders/RaytracingShaderColor_PAQ.hlsl.h"
//#include "Shaders/RaytracingShaderShadows_PAQ.hlsl.h"
#include "Shaders/VertexShaderCubes.hlsl.h"
#include "Shaders/VertexShaderEnvironmentMap.hlsl.h"
#include "Shaders/VertexShaderFullscreenQuad.hlsl.h"
#include "Shaders/VertexShaderMesh.hlsl.h"

//using namespace DirectX;
using namespace DX;
// RaytracingHlslCompat.h declares 'using' DirectX namespaces
//using namespace DirectX::SimpleMath;

Game::Game() noexcept(false) //: m_isRaster(true)
{
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.

    m_deviceResources           = std::make_unique<DeviceResources>(Globals::BackbufferFormat);
    //m_deviceResources         = std::make_unique<DX::DeviceResources>();

    m_deviceResources->RegisterDeviceNotify(this);

    //m_rayGenCB.viewport = { -1.0f, -1.0f, 1.0f, 1.0f };

    // Game object initialization.
    m_timer                     = std::make_unique<StepTimer>();
    //m_camera                    = std::make_unique<Camera>();
    m_keyboard                  = std::make_unique<Keyboard>();
    m_mouse                     = std::make_unique<Mouse>();
    m_mouseTracker              = std::make_unique<Mouse::ButtonStateTracker>();
    m_keyTracker                = std::make_unique<Keyboard::KeyboardStateTracker>();

    //m_frameConstants            = std::make_unique<FrameConstants>();
    m_constantBufferIndirect    = std::make_unique<ConstantBuffer<FrameConstants>>(); // Required for indirect drawing.
    //m_geometryTransforms      = std::make_unique<XMFLOAT3X4[]>(CUBE_INSTANCE_COUNT);
    //m_cubeConstants           = std::make_unique<ObjectConstants[]>(CUBE_INSTANCE_COUNT);
    //m_cubeTransforms          = std::make_unique<Matrix[]>(Globals::Cube_Instance_Count);
    //m_cubeTransforms            = std::make_unique<XMFLOAT3X4[]>(Globals::CubeInstanceCount);
    //m_cubeConstants           = std::make_unique<ObjectConstants[]>(Globals::Cube_Instance_Count);
    //m_planeConstants          = std::make_unique<ObjectConstants>();
    m_blurConstants             = std::make_unique<BlurConstants>();

    m_procGeometry              = std::make_unique<ProcGeometryBuffersAndViews[]>(ProcGeometries::Count);
    //m_cubeBuffers             = std::make_unique<GeometryBuffers>();
    //m_planeBuffers            = std::make_unique<GeometryBuffers>();

    //m_tlasBuffers               = std::make_unique<AccelerationStructureBuffers>();
    //m_blasBuffers[BLASType::Static]  = std::make_unique<AccelerationStructureBuffers[]>(StaticBLAS::Count);
    //m_blasBuffers[BLASType::Dynamic] = std::make_unique<AccelerationStructureBuffers[]>(DynamicBLAS::Count);
    //m_blasBuffers               = std::make_unique<AccelerationStructureBuffers[]>(MeshGeometries::Count);

    //m_geometryDesc              = std::make_unique<D3D12_RAYTRACING_GEOMETRY_DESC[]>(MeshGeometries::Count);
    //m_tlasInstanceDesc    = std::make_unique<D3D12_RAYTRACING_INSTANCE_DESC[]>(TLASInstances::Count); // Make shared?
    //m_instanceDescs           = std::make_unique< D3D12_RAYTRACING_INSTANCE_DESC[]>(m_geometryInstanceCount + 1);

    m_shaderBindingTableGroup   = std::make_unique<ShaderBindingTableGroup[]>(StateObjects::HitGroupCollection);

    // Render targets.
    for (uint32_t i = 0; i < RenderTextures::Count; i++)
    {
        m_renderTex[i] = i == RenderTextures::Tonemap
            ? std::make_unique<RenderTexture>(Globals::BackbufferFormat)
            : std::make_unique<RenderTexture>(Globals::HDRFormat);

        //switch (i)
        //{
        //    case RenderTextures::Hdr:
        //    case RenderTextures::Pass1:
        //    case RenderTextures::Pass2:
        //    case RenderTextures::Bloom:
        //        m_renderTex[i] = std::make_unique<RenderTexture>(Globals::HDRFormat);
        //        break;
        //    case RenderTextures::Tonemap:
        //        m_renderTex[i] = std::make_unique<RenderTexture>(Globals::BackbufferFormat);
        //        break;
        //    default:
        //        break;
        //}
    }
    //m_hdrTex = std::make_unique<RenderTexture>(Globals::HDRFormat);

    //m_shaderTableRadiance     = std::make_unique<ShaderTableSection>();
    //m_shaderTableAmbientOcclusion = std::make_unique<ShaderTableSection>();

    //m_missShaderTable         = std::make_unique<ShaderTableBuffer>();
    //m_hitGroupShaderTable     = std::make_unique<ShaderTableBuffer>();
    //m_rayGenShaderTable       = std::make_unique<ShaderTableBuffer>();

    //m_missShaderTable_ao      = std::make_unique<ShaderTableBuffer>();
    //m_hitGroupShaderTable_ao  = std::make_unique<ShaderTableBuffer>();
    //m_rayGenShaderTable_ao    = std::make_unique<ShaderTableBuffer>();
}

Game::~Game()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);
    m_mouse->SetWindow(window);
    UpdateForSizeChange(width, height);

    m_deviceResources->CreateDeviceResources();

    //ThrowIfFalse(IsDirectXRaytracingSupported(m_deviceResources->GetAdapter()),
    //    L"ERROR: DirectX Raytracing is not supported by your OS, GPU and/or driver.\n\n");

    CreateDeviceDependentResources();

    m_mainScene = std::make_unique<SceneMain>(this, true);
    //m_mainScene = std::make_unique<SceneMain>(m_deviceResources.get(), m_timer.get());
    //m_mainScene = std::make_unique<SceneMain>(this, m_deviceResources.get(), m_timer.get());

    m_scene = m_mainScene.get();

    // Create window size dependent resources after acquiring the scene pointer, because we need the scene's camera 
    // in order to set its projection matrix.
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    //// Set the camera's initial position.
    //m_camera->LookAt(Vector3(0, 3, 15), Vector3::Zero, Vector3::Up);
    //m_camera->UpdateViewMatrix();
    //
    //// Set the camera's bounding sphere.
    ////m_cameraSphere.Center = m_camera->GetPosition3f();
    ////m_cameraSphere.Radius = 1;
    //
    //// Set directional light attributes.
    //m_frameConstants->lightDiffuse = Vector4(1.1f, 1.1f, 0.8f, 1);
    //m_frameConstants->lightAmbient = Vector4(0.1f, 0.1f, 0.25f, 1);
    //m_frameConstants->lightDir = Vector3(0, -1, -1);
    //m_frameConstants->lightDir.Normalize();

    // Get offset vectors for ao raytracing
    //m_ssao->GetOffsetVectors(m_frameConstants.offsetVectors);

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer->Tick([&]()
        {
            m_scene->Update();
        });
    m_scene->Render();
    m_scene->CalculateFrameStats();
}

// Helper method to clear the back buffers or alternative render targets.
void Game::Clear(ID3D12GraphicsCommandList* commandList)
{
    //auto commandList = m_deviceResources->GetCommandList();
    //PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    const auto renderTex = m_renderTex[RenderTextures::Hdr].get();

    // PIX reports a warning for consecutive calls to ResourceBarrier, because the Prepare() method in DeviceResources
    // also includes a resource barrier to transition the current backbuffer to 'render target' state.
    renderTex->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //m_hdrTex->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Uncomment after adding dxr pipeline
    if (m_scene->GetRasterFlag())
    //if (m_isRaster)
    {
        // Clear the views.
        const auto rtvDescriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::HdrRtv);
        //auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
        const auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

        //commandList->OMSetRenderTargets(1u, &rtvDescriptor, FALSE, nullptr);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);

        renderTex->Clear(commandList);
        //m_hdrTex->Clear(commandList);
        //commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
        commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 0, 0, 0, nullptr); // Reverse-z buffer.
        //commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    }

    // Set the viewport and scissor rect.
    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    //PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer->ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    UpdateForSizeChange(width, height);
    CreateWindowSizeDependentResources();
}

// Update the application state with the new resolution.
void Game::UpdateForSizeChange(int width, int height)
{
    //DXSample::UpdateForSizeChange(width, height);

    m_width = width;
    m_height = height;
    m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);

    //float border = 0.1f;
    //if (m_width <= m_height)
    //{
    //    m_rayGenCB.stencil =
    //    {
    //        -1 + border, -1 + border * m_aspectRatio,
    //        1.0f - border, 1 - border * m_aspectRatio
    //    };
    //}
    //else
    //{
    //    m_rayGenCB.stencil =
    //    {
    //        -1 + border / m_aspectRatio, -1 + border,
    //         1 - border / m_aspectRatio, 1.0f - border
    //    };
    //
    //}
}

void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1600; // 800;
    height = 900; // 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    const auto device       = m_deviceResources->GetD3DDevice();
    //const auto commandList  = m_deviceResources->GetCommandList();
    //const auto commandAlloc = m_deviceResources->GetCommandAllocator();

    // Check Shader Model 6.6 support.
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.6 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.6 is not supported!");
    }

    // Check DXR support (added this).
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))
        || (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: DXR is not supported!\n");
#endif
        throw std::runtime_error("DXR is not supported!");
    }

    // Check Enhanced Barriers support.
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12)))
        || (options12.EnhancedBarriersSupported == false))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Enhanced Barriers are not supported!\n");
#endif
        throw std::runtime_error("Enhanced Barriers are not supported!");
    }

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // TODO: Initialize device dependent objects here (independent of window size).

    // Set common states.
    //m_commonStates = std::make_unique<CommonStates>(device); // states object includes common samplers

    // Set descriptor heaps.
    m_descHeap[DescriptorHeaps::SrvUav] = std::make_unique<DescriptorHeap>(device, SrvUAVs::Count);
    m_descHeap[DescriptorHeaps::Rtv]    = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTVs::Count);
    //m_srvUavHeap = std::make_unique<DescriptorHeap>(device, SrvUavDescriptors::Count);
    //m_rtvHeap = std::make_unique<DescriptorHeap>(device,
    // D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    // D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RtvDescriptors::Count);

    // Initialize render textures.
    //XMVECTOR clearColor = { 0.0f, 0.2f, 0.4f, 1.0f };
    XMVECTORF32 color = { 0, 0, 0, 0 };
    //XMVECTORF32 color = {};
    //color.v = XMColorSRGBToRGB(clearColor);
    //color.v = XMColorSRGBToRGB(Colors::Black);
    m_renderTex[RenderTextures::Hdr]->SetClearColor(color);
    m_renderTex[RenderTextures::Pass1]->SetClearColor(color);
    m_renderTex[RenderTextures::Pass2]->SetClearColor(color);
    m_renderTex[RenderTextures::Bloom]->SetClearColor(color);
    m_renderTex[RenderTextures::Tonemap]->SetClearColor(color);

    m_renderTex[RenderTextures::Hdr]->SetDevice(device,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::HdrSrv),
        m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::HdrRtv));
    m_renderTex[RenderTextures::Pass1]->SetDevice(device,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::Pass1Srv),
        m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::Pass1Rtv));
    m_renderTex[RenderTextures::Pass2]->SetDevice(device,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::Pass2Srv),
        m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::Pass2Rtv));
    m_renderTex[RenderTextures::Bloom]->SetDevice(device,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::BloomSrv),
        m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::BloomRtv));
    m_renderTex[RenderTextures::Tonemap]->SetDevice(device,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::TonemapSrv),
        m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::TonemapRtv));

    // Set render states.
    const auto hdrState = RenderTargetState(Globals::HDRFormat, Globals::DepthFormat);        // Scene render state.
    const auto ppState  = RenderTargetState(Globals::HDRFormat, DXGI_FORMAT_UNKNOWN);         // Postprocess render state.
    const auto rtState  = RenderTargetState(Globals::BackbufferFormat, DXGI_FORMAT_UNKNOWN);  // Backbuffer render target state.
    //auto rtState  = RenderTargetState(Globals::BackbufferFormat, Globals::DepthFormat);

    // Set post-processors.
    m_basicPostProc[PostProcessType::BloomExtract] = std::make_unique<BasicPostProcess>(device, ppState, BasicPostProcess::BloomExtract);
    m_basicPostProc[PostProcessType::BloomBlur]    = std::make_unique<BasicPostProcess>(device, ppState, BasicPostProcess::BloomBlur);
    m_basicPostProc[PostProcessType::Blur]         = std::make_unique<BasicPostProcess>(device, ppState, BasicPostProcess::GaussianBlur_5x5);
    m_basicPostProc[PostProcessType::Copy]         = std::make_unique<BasicPostProcess>(device, ppState, BasicPostProcess::Copy);

    // Dual post-processors.
    m_bloomCombine = std::make_unique<DualPostProcess>(device, ppState, DualPostProcess::BloomCombine);

    // Set tone-mappers.
    m_toneMap[TonemapType::Default] = std::make_unique<ToneMapPostProcess>(device, rtState,
        ToneMapPostProcess::ACESFilmic, // ACES Filmic operator
        //ToneMapPostProcess::Reinhard, // Reinhard local operator
        //ToneMapPostProcess::None,
        m_deviceResources->GetBackBufferFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT ?
        ToneMapPostProcess::Linear : ToneMapPostProcess::SRGB);

    m_toneMap[TonemapType::HDR10]   = std::make_unique<ToneMapPostProcess>(device, rtState,
        ToneMapPostProcess::ACESFilmic, // ACES Filmic operator
        //ToneMapPostProcess::Reinhard, // Reinhard local operator
        //ToneMapPostProcess::None,
        ToneMapPostProcess::ST2084);

    m_toneMap[TonemapType::Linear]  = std::make_unique<ToneMapPostProcess>(device, rtState,
        ToneMapPostProcess::ACESFilmic, // ACES Filmic operator
        //ToneMapPostProcess::Reinhard, // Reinhard local operator
        //ToneMapPostProcess::None,
        ToneMapPostProcess::Linear);

    //
    // Create root signatures.
    //
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Create graphics root signature.
    {
        //const D3D12_STATIC_SAMPLER_DESC staticSamplers[] = {
        //    CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC), // shader register, filter
        //    CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR),
        //};

        //const D3D12_STATIC_SAMPLER_DESC staticSamplers[] = {
        //    CommonStates::StaticAnisotropicClamp(0, D3D12_SHADER_VISIBILITY_PIXEL),
        //    CommonStates::StaticLinearClamp(1, D3D12_SHADER_VISIBILITY_PIXEL),
        //    //m_commonStates->StaticAnisotropicClamp(0, D3D12_SHADER_VISIBILITY_PIXEL),
        //    //m_commonStates->StaticLinearClamp(1, D3D12_SHADER_VISIBILITY_PIXEL),
        //};

        //CD3DX12_DESCRIPTOR_RANGE objectSrvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameters[GraphicsRootSigParams::Count] = {};
        //CD3DX12_ROOT_PARAMETER rootParameters[GraphicsRootSigParams::Count] = {};
        rootParameters[GraphicsRootSigParams::FrameCB].InitAsConstantBufferView(GraphicsRootSigParams::FrameCB,0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
        rootParameters[GraphicsRootSigParams::MeshCB].InitAsConstantBufferView(GraphicsRootSigParams::MeshCB, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
        //rootParameters[GraphicsRootSigParams::BoneCB].InitAsConstantBufferView(GraphicsRootSigParams::BoneCB, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
        //rootParameters[GraphicsRootSigParams::FrameConstants].InitAsConstantBufferView(GraphicsRootSigParams::FrameConstants);
        //rootParameters[GraphicsRootSigParams::ObjectConstants].InitAsConstantBufferView(GraphicsRootSigParams::ObjectConstants, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        //rootParameters[GraphicsRootSigParams::BoneConstants].InitAsConstantBufferView(GraphicsRootSigParams::BoneConstants,     0, D3D12_SHADER_VISIBILITY_VERTEX);

        //rootParameters[RasterRootSigParams::ObjectConstantSlot].InitAsConstants(
        // SizeOfInUint32(ObjectConstants), RasterRootSigParams::ObjectConstantSlot, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        //rootParameters[RasterRootSigParams::ObjectDescriptorTableSlot].InitAsDescriptorTable(
        // 1, &objectSrvRange, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_ROOT_SIGNATURE_FLAGS rsFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        const auto rootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
            ARRAYSIZE(rootParameters), rootParameters, ARRAYSIZE(m_staticSamplers), m_staticSamplers, rsFlags);
        //auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
        //auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
        //    ARRAYSIZE(rootParameters), rootParameters, ARRAYSIZE(m_staticSamplers), m_staticSamplers, rsFlags);
        //rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
        ThrowIfFailed(CreateVersionedRootSignature(device, &rootSignatureDesc, featureData.HighestVersion, &m_rootSig[RootSignatures::Graphics]));
        //ThrowIfFailed(CreateRootSignature(device, &rootSignatureDesc, &m_rootSig[RootSignatures::Graphics]));
    }

    // Create compute root signatures.
    // Since we have two separate compute shaders per blur stage, we can hard code the blur direction and
    // therefore we don't require the second root parameter slot.
    // Edit: We are replacing individual compute root signatures for blur, skinning and for the global DXR signature
    // with a single compute rootsig. 
    {
        CD3DX12_DESCRIPTOR_RANGE1 srvRange = {};
        srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // Base  shader register is t0.
        //srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); // base  shader register is t1

        CD3DX12_ROOT_PARAMETER1 rootParameters[ComputeRootSigParams::Count] = {};
        //CD3DX12_ROOT_PARAMETER rootParameters[ComputeRootSigParams::Count] = {};
        //CD3DX12_ROOT_PARAMETER rootParameters = {};
        //rootParameters.InitAsConstantBufferView(0);

        rootParameters[ComputeRootSigParams::FrameCB].InitAsConstantBufferView(ComputeRootSigParams::FrameCB, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
        rootParameters[ComputeRootSigParams::BoneCB].InitAsConstantBufferView(ComputeRootSigParams::BoneCB, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
        //rootParameters[ComputeRootSigParams::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);// t0
        rootParameters[ComputeRootSigParams::IndirectSrvTable].InitAsDescriptorTable(1, &srvRange); // t0
        //rootParameters[ComputeRootSigParams::FrameConstants].InitAsConstantBufferView(0);       // b0
        //rootParameters[ComputeRootSigParams::AccelerationStructure].InitAsShaderResourceView(0);// t0
        //rootParameters[1].InitAsConstants(1, 1);

        D3D12_ROOT_SIGNATURE_FLAGS rsFlags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
        const auto rootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(
            ARRAYSIZE(rootParameters), rootParameters, ARRAYSIZE(m_staticSamplers), m_staticSamplers, rsFlags);
        //auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
        //    ARRAYSIZE(rootParameters), rootParameters, ARRAYSIZE(m_staticSamplers), m_staticSamplers, rsFlags);
        //auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(ARRAYSIZE(rootParameters), rootParameters, 0, nullptr, rsFlags);
        //auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(1, &rootParameters, 0, nullptr, rsFlags);
        //CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(static_cast<UINT>(std::size(rootParameters)), rootParameters,
        // 0, nullptr , rsFlags);

        // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
        ThrowIfFailed(CreateVersionedRootSignature(device, &rootSignatureDesc, featureData.HighestVersion, &m_rootSig[RootSignatures::Compute]));
        //ThrowIfFailed(CreateRootSignature(device, &rootSignatureDesc, &m_rootSig[RootSignatures::Compute]));
        //ThrowIfFailed(CreateRootSignature(device, &rootSignatureDesc, &m_rootSig[RootSignatures::ComputeBlur]));
    }
    
    // Compute root signature for vertex skinning shader.
    //{
    //    CD3DX12_ROOT_PARAMETER rootParameters = {};
    //    rootParameters.InitAsConstantBufferView(0);
    //
    //    D3D12_ROOT_SIGNATURE_FLAGS rsFlags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
    //    auto rootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(1, &rootParameters, 0, nullptr, rsFlags);
    //
    //    // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
    //    ThrowIfFailed(CreateRootSignature(device, &rootSignatureDesc, &m_rootSig[RootSignatures::ComputeSkinning]));
    //}

    // Local Root Signatures for color and ambient occlusion ray tracing passes
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    // NB: Local arguments need to be passed as root constants.
    // Edit: We are now indexing into a structured buffer to obtain local arguments, so default local rootsig is reinstated.

    /*    // Local root signature for DXR pipeline.
    {
        //CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSigParams::Count]{};
        // // b0 was bound in global root sig.
        //rootParameters[LocalRootSigParams::MaterialConstantSlot].InitAsConstants(SizeOfInUint32(ObjectConstants), 1, 0);
        //rootParameters[LocalRootSigParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
        
        // Just create a default local rootsig since we are not binding any local resources.
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
        //auto rootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
        //auto localRootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
        //CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        //localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
        ThrowIfFailed(CreateVersionedRootSignature(device, &rootSignatureDesc, featureData.HighestVersion, &m_rootSig[RootSignatures::DxrLocal]));
        //ThrowIfFailed(CreateRootSignature(device, &localRootSignatureDesc, &m_rootSig[RootSignatures::DxrLocal]));
        //SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
    }   // */

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        // Replaced C style cast (void*) with C++ style cast.
        const auto computeAOBlurHorz     = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_ComputeShaderAOBlurHorz), ARRAYSIZE(g_ComputeShaderAOBlurHorz));
        const auto computeAOBlurVert     = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_ComputeShaderAOBlurVert), ARRAYSIZE(g_ComputeShaderAOBlurVert));
        const auto computePostProcess    = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_ComputeShaderFinalPostProcess), ARRAYSIZE(g_ComputeShaderFinalPostProcess));
        const auto computeShadowBlurHorz = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_ComputeShaderShadowBlurHorz), ARRAYSIZE(g_ComputeShaderShadowBlurHorz));
        const auto computeShadowBlurVert = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_ComputeShaderShadowBlurVert), ARRAYSIZE(g_ComputeShaderShadowBlurVert));
        const auto computeSkinning       = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_ComputeShaderSkinning), ARRAYSIZE(g_ComputeShaderSkinning));
        const auto pixelShaderCubes      = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_PixelShaderCubes), ARRAYSIZE(g_PixelShaderCubes));
        const auto pixelShaderEnvMap     = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_PixelShaderEnvironmentMap), ARRAYSIZE(g_PixelShaderEnvironmentMap));
        const auto pixelShaderFxaa       = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_PixelShaderFxaa), ARRAYSIZE(g_PixelShaderFxaa));
        const auto pixelShaderMesh       = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_PixelShaderMesh), ARRAYSIZE(g_PixelShaderMesh));
        const auto vertexShaderCubes     = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_VertexShaderCubes), ARRAYSIZE(g_VertexShaderCubes));
        const auto vertexShaderEnvMap    = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_VertexShaderEnvironmentMap), ARRAYSIZE(g_VertexShaderEnvironmentMap));
        const auto vertexShaderQuad      = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_VertexShaderFullscreenQuad), ARRAYSIZE(g_VertexShaderFullscreenQuad));
        const auto vertexShaderMesh      = CD3DX12_SHADER_BYTECODE(reinterpret_cast<const void*>(g_VertexShaderMesh), ARRAYSIZE(g_VertexShaderMesh));

        // Create input layouts.
        const D3D12_INPUT_ELEMENT_DESC inputElementDescInstanced[] =
        {
            // VertexPositionNormalTexture in slot #0, XMFLOAT3X4 in slot #1, color in slot #3
            { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   0 },
            { "INSTMATRIX",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTMATRIX",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTMATRIX",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            //{ "INSTCOLOR",   0, DXGI_FORMAT_R32G32B32_FLOAT,    2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            //{ "INSTCOLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        };
        const D3D12_INPUT_LAYOUT_DESC inputLayoutInstanced = { inputElementDescInstanced, ARRAYSIZE(inputElementDescInstanced) };
        //const D3D12_INPUT_LAYOUT_DESC inputLayoutInstanced = { inputElementDescInstanced, static_cast<uint32_t>(std::size(inputElementDescInstanced)) };

        const D3D12_INPUT_ELEMENT_DESC inputElementDescTangent[] =
        {
            { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        const D3D12_INPUT_LAYOUT_DESC inputLayoutTangent = { inputElementDescTangent, ARRAYSIZE(inputElementDescTangent) };
        //const D3D12_INPUT_LAYOUT_DESC inputLayoutTangent = { inputElementDescTangent, static_cast<uint32_t>(std::size(inputElementDescTangent)) };

        const D3D12_INPUT_ELEMENT_DESC inputElementDescSkinned[] =
        {
            { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "BONEWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            // DXGI _INT & _UINT formats unpack to HLSL int & uint in the shader.
            // DXGI_FORMAT_R8G8B8A8_UINT unpacks to HLSL uint4.
            { "BONEINDEX",   0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        const D3D12_INPUT_LAYOUT_DESC inputLayoutSkinned = { inputElementDescSkinned, ARRAYSIZE(inputElementDescSkinned) };
        //const D3D12_INPUT_LAYOUT_DESC inputLayoutSkinned = { inputElementDescSkinned, static_cast<uint32_t>(std::size(inputElementDescSkinned)) };

        // Create graphics pipeline state objects (PSOs), utilizing multithreading.
        std::vector<std::thread> graphicsPsoThreads;

        const auto pdCubes = EffectPipelineStateDescription(
            &inputLayoutInstanced,
            CommonStates::Opaque,
            CommonStates::DepthReverseZ,
            //CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            hdrState
        );
        graphicsPsoThreads.push_back(std::thread(CreateGraphicsPipelineStateOnWorkerThread, pdCubes,
        //pdCubes.CreatePipelineState(
            device,
            m_rootSig[RootSignatures::Graphics].Get(),
            vertexShaderCubes,
            pixelShaderCubes,
            &m_pipelineState[PSOs::Cubes]
        ));

        //auto pdGroundPlane = EffectPipelineStateDescription(
        //    &VertexPositionNormalTexture::InputLayout,
        //    //&VertexPositionNormal::InputLayout,
        //    CommonStates::AlphaBlend,
        //    CommonStates::DepthReverseZ,
        //    //CommonStates::DepthDefault,
        //    CommonStates::CullCounterClockwise,
        //    hdrState
        //);
        //pdGroundPlane.CreatePipelineState(
        //    device,
        //    m_rootSig[RootSignatures::Graphics].Get(),
        //    vertexShaderPlane,
        //    pixelShaderPlane,
        //    &m_pipelineState[PSOs::Plane]
        //);

        const auto pdOpaque = EffectPipelineStateDescription(
            &inputLayoutTangent,
            CommonStates::Opaque,
            CommonStates::DepthReverseZ,
            //CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            hdrState
        );
        graphicsPsoThreads.push_back(std::thread(CreateGraphicsPipelineStateOnWorkerThread, pdOpaque,
            //pdOpaque.CreatePipelineState(
            device,
            m_rootSig[RootSignatures::Graphics].Get(),
            vertexShaderMesh,
            pixelShaderMesh,
            &m_pipelineState[PSOs::MeshOpaque]
        ));

        const auto pdAlphaBlend = EffectPipelineStateDescription(
            &inputLayoutTangent,
            CommonStates::NonPremultiplied,
            //CommonStates::AlphaBlend,
            CommonStates::DepthReverseZ,
            CommonStates::CullNone,
            //CommonStates::CullCounterClockwise,
            hdrState
        );
        graphicsPsoThreads.push_back(std::thread(CreateGraphicsPipelineStateOnWorkerThread, pdAlphaBlend,
            //pdAlphaBlend.CreatePipelineState(
            device,
            m_rootSig[RootSignatures::Graphics].Get(),
            vertexShaderMesh,
            pixelShaderMesh,
            &m_pipelineState[PSOs::MeshAlphaBlend]
        ));

        const auto pdGeoSphere = EffectPipelineStateDescription(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthReadReverseZ,
            //CommonStates::DepthNone, // When geosphere cubemap is drawn first, no depth writes or reads are required.
            //depthStencilDescGeoSphere,
            CommonStates::CullNone,
            hdrState
        );
        graphicsPsoThreads.push_back(std::thread(CreateGraphicsPipelineStateOnWorkerThread, pdGeoSphere,
            //pdGeoSphere.CreatePipelineState(
            device,
            m_rootSig[RootSignatures::Graphics].Get(),
            vertexShaderEnvMap,
            pixelShaderEnvMap,
            &m_pipelineState[PSOs::GeoSphere]
        ));

        const auto pdFxaa = EffectPipelineStateDescription(
            &VertexPositionTexture::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtState);
        graphicsPsoThreads.push_back(std::thread(CreateGraphicsPipelineStateOnWorkerThread, pdFxaa,
            //pdFxaa.CreatePipelineState(
            device,
            m_rootSig[RootSignatures::Graphics].Get(),
            vertexShaderQuad,
            pixelShaderFxaa,
            &m_pipelineState[PSOs::Fxaa]
        ));

        for (auto& t : graphicsPsoThreads)
            t.join();

        // Create compute pipeline state objects (PSOs), utilizing multithreading.
        std::vector<std::thread> computePsoThreads;

        // Bilateral blur compute PSOs.
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoAOBlurHorzDesc = {};
        psoAOBlurHorzDesc.pRootSignature = m_rootSig[RootSignatures::Compute].Get();
        //psoSsaoHorzBlurDesc.pRootSignature = m_rootSig[RootSignatures::ComputeBlur].Get();
        psoAOBlurHorzDesc.CS = computeAOBlurHorz;
        psoAOBlurHorzDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computePsoThreads.push_back(std::thread(CreateComputePipelineStateOnWorkerThread, device, &psoAOBlurHorzDesc, &m_pipelineState[PSOs::AOBlurHorz]));
        //ThrowIfFailed(device->CreateComputePipelineState(&psoAOBlurHorzDesc, IID_PPV_ARGS(&m_pipelineState[PSOs::AOBlurHorz])));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoAOBlurVertDesc = {};
        psoAOBlurVertDesc.pRootSignature = m_rootSig[RootSignatures::Compute].Get();
        psoAOBlurVertDesc.CS = computeAOBlurVert;
        psoAOBlurVertDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computePsoThreads.push_back(std::thread(CreateComputePipelineStateOnWorkerThread, device, &psoAOBlurVertDesc, &m_pipelineState[PSOs::AOBlurVert]));
        //ThrowIfFailed(device->CreateComputePipelineState(&psoAOBlurVertDesc, IID_PPV_ARGS(&m_pipelineState[PSOs::AOBlurVert])));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoShadowBlurHorzDesc = {};
        psoShadowBlurHorzDesc.pRootSignature = m_rootSig[RootSignatures::Compute].Get();
        psoShadowBlurHorzDesc.CS = computeShadowBlurHorz;
        psoShadowBlurHorzDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computePsoThreads.push_back(std::thread(CreateComputePipelineStateOnWorkerThread, device, &psoShadowBlurHorzDesc, &m_pipelineState[PSOs::ShadowBlurHorz]));
        //ThrowIfFailed(device->CreateComputePipelineState(&psoShadowBlurHorzDesc, IID_PPV_ARGS(&m_pipelineState[PSOs::ShadowBlurHorz])));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoShadowBlurVertDesc = {};
        psoShadowBlurVertDesc.pRootSignature = m_rootSig[RootSignatures::Compute].Get();
        psoShadowBlurVertDesc.CS = computeShadowBlurVert;
        psoShadowBlurVertDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computePsoThreads.push_back(std::thread(CreateComputePipelineStateOnWorkerThread, device, &psoShadowBlurVertDesc, &m_pipelineState[PSOs::ShadowBlurVert]));
        //ThrowIfFailed(device->CreateComputePipelineState(&psoShadowBlurVertDesc, IID_PPV_ARGS(&m_pipelineState[PSOs::ShadowBlurVert])));

        // Vertex skinning compute PSO.
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoComputeSkinDesc = {};
        psoComputeSkinDesc.pRootSignature = m_rootSig[RootSignatures::Compute].Get();
        //psoComputeSkinDesc.pRootSignature = m_rootSig[RootSignatures::ComputeSkinning].Get();
        psoComputeSkinDesc.CS = computeSkinning;
        psoComputeSkinDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computePsoThreads.push_back(std::thread(CreateComputePipelineStateOnWorkerThread, device, &psoComputeSkinDesc, &m_pipelineState[PSOs::Skinning]));
        //ThrowIfFailed(device->CreateComputePipelineState(&psoComputeSkinDesc, IID_PPV_ARGS(&m_pipelineState[PSOs::Skinning])));

        // Postprocess compute PSO.
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoPostProcessDesc = {};
        psoPostProcessDesc.pRootSignature = m_rootSig[RootSignatures::Compute].Get();
        psoPostProcessDesc.CS = computePostProcess;
        psoPostProcessDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        computePsoThreads.push_back(std::thread(CreateComputePipelineStateOnWorkerThread, device, &psoPostProcessDesc, &m_pipelineState[PSOs::PostProcess]));
        //ThrowIfFailed(device->CreateComputePipelineState(&psoPostProcessDesc, IID_PPV_ARGS(&m_pipelineState[PSOs::PostProcess])));

        for (auto& t : computePsoThreads)
            t.join();
    }

    // Create the command signature used for indirect drawing.
    {
        // Each command consists of a CBV update and a DrawInstanced call.
        D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[2] = {};
        argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
        argumentDescs[0].ConstantBufferView.RootParameterIndex = GraphicsRootSigParams::FrameCB;
        argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;// D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

        D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
        commandSignatureDesc.pArgumentDescs   = argumentDescs;
        commandSignatureDesc.NumArgumentDescs = ARRAYSIZE(argumentDescs);
        commandSignatureDesc.ByteStride       = sizeof(IndirectCommand);
        ThrowIfFailed(device->CreateCommandSignature(&commandSignatureDesc, m_rootSig[RootSignatures::Graphics].Get(),
            IID_PPV_ARGS(&m_commandSignature)));
    }

    // Create root signatures for the shaders.
    //CreateRootSignatures(device);

    // Initialize RTAO and shadow raytracing instances.

    //auto commandList = m_deviceResources->GetCommandList();
    //auto commandAlloc = m_deviceResources->GetCommandAllocator();
    //auto commandQueue = m_deviceResources->GetCommandQueue();

    // Reset command list and allocator.
    //ThrowIfFailed(commandAlloc->Reset());
    //ThrowIfFailed(commandList->Reset(commandAlloc, nullptr));

    const auto cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_aoManager = std::make_unique<OcclusionManager>(
        device,
        //m_rootSig[RootSignatures::ComputeBlur].Get(),
        m_pipelineState[PSOs::AOBlurHorz].Get(),
        m_pipelineState[PSOs::AOBlurVert].Get(),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::AmbientBuffer0Uav)),
        cbvSrvUavDescriptorSize, true, true
    );
    //m_ssao = std::make_unique<Ssao>(device, commandList);

    // Execute the SSAO initialization commands.
    //ThrowIfFailed(commandList->Close());
    //ID3D12CommandList* cmdsLists[] = { commandList };
    //commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    m_shadowManager = std::make_unique<OcclusionManager>(
        device,
        m_pipelineState[PSOs::ShadowBlurHorz].Get(),
        m_pipelineState[PSOs::ShadowBlurVert].Get(),
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::ShadowBuffer0Uav)),
        cbvSrvUavDescriptorSize, false, false
    );

    CreateGeometryAndMaterialResources(device);
    CreateRaytracingResources(device);
    //BuildGeometryAndMaterials(device, commandQueue);

    //m_deviceResources->WaitForGpu(); // wait until initialization is complete
    // Descriptor and blur weight builds for occlusion managers have been moved inside their constructors.

    //m_raytracedAO->BuildDescriptors(
    //    CD3DX12_CPU_DESCRIPTOR_HANDLE(
    //        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::AmbientBuffer0Uav)),
    //    //CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvUavHeap->GetGpuHandle(SrvUavDescriptors::AmbientMap0Uav)),
    //    cbvSrvUavDescriptorSize
    //);
    //m_raytracedShadows->BuildDescriptors(
    //    CD3DX12_CPU_DESCRIPTOR_HANDLE(
    //        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::ShadowBuffer0Uav)),
    //    cbvSrvUavDescriptorSize
    //);

    // Do not build Rtao resources here. They are window size-dependent resources.
    //m_rtao->OnResize(m_width, m_height);

    //m_rtao->SetRootSigAndPSOs(
    // m_rootSig[RootSignatures::ComputeBlur].Get(),
    // m_pipelineState[PipelineStates::BlurHorz].Get(),
    // m_pipelineState[PipelineStates::BlurVert].Get());

    //const auto blurWeights = m_raytracedAO->CalcGaussWeights(2.5f); // calculates the max blur radius of 5.f
    //m_blurConstants->blurWeights[0] = Vector4(&blurWeights[0]);
    //m_blurConstants->blurWeights[1] = Vector4(&blurWeights[4]);
    //m_blurConstants->blurWeights[2] = Vector4(&blurWeights[8]);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    const auto device = m_deviceResources->GetD3DDevice();
    const auto commandList = m_deviceResources->GetCommandList();

    // Get current backbuffer dimensions.
    const auto outputRect = m_deviceResources->GetOutputSize();
    const auto quarterRect= CD3DX12_RECT(0, 0, outputRect.right / 2, outputRect.bottom / 2);

    // TODO: Initialize windows-size dependent objects here.

    // Resize render targets.
    m_renderTex[RenderTextures::Hdr]->SetWindow(outputRect);
    m_renderTex[RenderTextures::Tonemap]->SetWindow(outputRect);
    m_renderTex[RenderTextures::Pass1]->SetWindow(quarterRect);
    m_renderTex[RenderTextures::Pass2]->SetWindow(quarterRect);
    m_renderTex[RenderTextures::Bloom]->SetWindow(quarterRect);
    //m_hdrTex->SetWindow(outputRect);

    // Set camera projection matrix.
    const auto camera = m_scene->GetCamera();
    camera->SetLens(1.f, m_aspectRatio, 5000, 0.1f);
    //m_camera->SetLens(1.f, m_aspectRatio, 100.f, 0.1f); // Swap near & far plane for reverse-z buffer.
    //m_camera->SetLens(1.f, m_aspectRatio, 0.1f, 100.f, m_width, m_height); // one radian field of view
    //m_camera->SetLens(1.f, m_aspectRatio, 0.1f, 100.f); // one radian field of view

    // TODO: Game window is being resized.
    CreateRaytracingOutputResources(device);

    // Rebuild occlusion raytracing resources, which are rendered at quarter-resolution.
    const int halfWidth  = m_width / 2;
    const int halfHeight = m_height / 2;

    m_aoManager->OnResize(halfWidth, halfHeight);
    m_shadowManager->OnResize(halfWidth, halfHeight);
    //m_rtao->OnResize(m_width, m_height);
    //m_rtShadow->OnResize(m_width, m_height);
}

void Game::CreateRaytracingResources(ID3D12Device5* device)
{
    //const auto device = m_deviceResources->GetD3DDevice();
    //const auto commandList = m_deviceResources->GetCommandList();
    //const auto commandAlloc = m_deviceResources->GetCommandAllocator();

    // Create a raytracing collection type that compiles common subobjects.
    {
        CD3DX12_STATE_OBJECT_DESC raytracingCollection{ D3D12_STATE_OBJECT_TYPE_COLLECTION };

        // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
        for (uint32_t meshType = 0; meshType < MeshType::Count; meshType++)
        {
            for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
            {
                auto hitGroup = raytracingCollection.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                if (rayType == RayType::Radiance)
                {
                    switch (meshType)
                    {
                    case MeshType::Cube:
                        hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[0]);
                        //hitGroup->SetAnyHitShaderImport(c_anyHitShaderNames[MeshType::Cube]);
                        hitGroup->SetHitGroupExport(c_hitGroupNames[meshType][rayType]);
                        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                        break;
                    case MeshType::Opaque:
                        hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[1]);
                        //hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[i]);
                        //hitGroup->SetAnyHitShaderImport(c_anyHitShaderNames[MeshType::Opaque]);
                        hitGroup->SetHitGroupExport(c_hitGroupNames[meshType][rayType]);
                        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                        break;
                    case MeshType::Transparent:
                        hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[1]);
                        //hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[i]);
                        hitGroup->SetAnyHitShaderImport(c_anyHitShaderName);
                        //hitGroup->SetAnyHitShaderImport(c_anyHitShaderNames[rayType]);
                        hitGroup->SetHitGroupExport(c_hitGroupNames[meshType][rayType]);
                        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                        break;
                    default:
                        break;
                    }
                }
                // Hit groups for occlusion rays
                else
                {
                    hitGroup->SetAnyHitShaderImport(c_anyHitShaderName);
                    //hitGroup->SetAnyHitShaderImport(c_anyHitShaderNames[rayType]);
                    hitGroup->SetHitGroupExport(c_hitGroupNames[meshType][rayType]);
                    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                }
            }
        }

        auto stateObjectConfig = raytracingCollection.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
        stateObjectConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_ALLOW_LOCAL_DEPENDENCIES_ON_EXTERNAL_DEFINITIONS);

        // Create the state object.
        ThrowIfFailed(device->CreateStateObject(
            raytracingCollection,
            IID_PPV_ARGS(&m_stateObject[StateObjects::HitGroupCollection])),
            L"Couldn't create DirectX Raytracing state object.\n"
        );
    }

    // Create a raytracing PSO which defines the binding of shaders, state and resources to be used during raytracing.
    // To use multithreading we must launch threads by passing a non-static function reference as the callable object.
    // Warning! A byte array passed to a function will "decay" to a pointer to just the first array element.
    // Therefore the array length must also be passed to the function.
    std::vector<std::thread> psoThreads;

    psoThreads.push_back(std::thread(&Game::CreateRaytracingPipelineStateObject, this,
    //CreateRaytracingPipelineStateObject(
        device,
        g_RaytracingShaderAO, ARRAYSIZE(g_RaytracingShaderAO),
        68u, //static_cast<uint32_t>(sizeof(AOPayload)),
        &m_stateObject[StateObjects::AOPipeline]
    ));
    psoThreads.push_back(std::thread(&Game::CreateRaytracingPipelineStateObject, this,
    //CreateRaytracingPipelineStateObject(
        device,
        g_RaytracingShaderShadows, ARRAYSIZE(g_RaytracingShaderShadows),
        52u, //static_cast<uint32_t>(sizeof(ShadowPayload)),
        &m_stateObject[StateObjects::ShadowPipeline]
    ));
    psoThreads.push_back(std::thread(&Game::CreateRaytracingPipelineStateObject, this,
    //CreateRaytracingPipelineStateObject(
        device,
        g_RaytracingShaderColor, ARRAYSIZE(g_RaytracingShaderColor),
        64u, //static_cast<uint32_t>(sizeof(ColorPayload)),
        &m_stateObject[StateObjects::MainPipeline]
    ));

    for (auto& t : psoThreads)
        t.join();
    //CreateRaytracingPipelineStateObject();

    // Build raytracing acceleration structures from the generated geometry.
    //BuildAccelerationStructures(device, commandList, commandAlloc);

    std::vector<std::thread> stThreads;

    // Original sample rebuilds the shader tables with window size dependent resources.
    stThreads.push_back(std::thread(&Game::BuildShaderTables, this,
    //BuildShaderTables(
        device,
        m_stateObject[StateObjects::AOPipeline],
        &m_shaderBindingTableGroup[StateObjects::AOPipeline]));
    stThreads.push_back(std::thread(&Game::BuildShaderTables, this,
    //BuildShaderTables(
        device,
        m_stateObject[StateObjects::ShadowPipeline],
        &m_shaderBindingTableGroup[StateObjects::ShadowPipeline]));
    stThreads.push_back(std::thread(&Game::BuildShaderTables, this,
    //BuildShaderTables(
        device,
        m_stateObject[StateObjects::MainPipeline],
        &m_shaderBindingTableGroup[StateObjects::MainPipeline]));

    for (auto& t : stThreads)
        t.join();
}

// Create 2D output textures for raytracing.
void Game::CreateRaytracingOutputResources(ID3D12Device10* device)
{
    //auto device = m_deviceResources->GetD3DDevice();
    const auto targetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    //auto targetFormat = m_hdrTex->GetFormat();
    //auto backbufferFormat = m_deviceResources->GetBackBufferFormat();

    // Create the output resource. The dimensions and format should match the swap-chain.
    const auto texDesc = CD3DX12_RESOURCE_DESC1::Tex2D(
        targetFormat /*backbufferFormat*/,
        m_width, m_height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    //auto uavDesc_ao = CD3DX12_RESOURCE_DESC::Tex2D(
    //    targetFormat /*backbufferFormat*/, m_width/2, m_height/2, 1, 1, 1, 0,
    // D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource3(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
        //D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr, nullptr, 0, nullptr,
        IID_PPV_ARGS(&m_raytracingOutput)
    ));
    NAME_D3D12_OBJECT(m_raytracingOutput);
    //ThrowIfFailed(device->CreateCommittedResource(
    //    &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc,
    // D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_raytracingOutput_ao)));
    //NAME_D3D12_OBJECT(m_raytracingOutput_ao);

    ThrowIfFailed(device->CreateCommittedResource3(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,
        nullptr, nullptr, 0, nullptr,
        IID_PPV_ARGS(&m_raytracingOutputPrevFrame)
    ));
    NAME_D3D12_OBJECT(m_raytracingOutputPrevFrame);
    // New DXTK method
    CreateUnorderedAccessView(
        device,
        m_raytracingOutput.Get(),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::DxrUav)
        );
    // SRVs for current and previous frame output textures.
    CreateShaderResourceView(
        device,
        m_raytracingOutput.Get(),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::DxrSrv)
    );
    CreateShaderResourceView(
        device,
        m_raytracingOutputPrevFrame.Get(),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::DxrPrevSrv)
    );

    //CD3DX12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle(m_srvUavHeap->GetCpuHandle(SrvUavDescriptors::DxrUav));
    //D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    //uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    //device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &uavDesc,
    // m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUavDescriptors::DxrUav));
    //device->CreateUnorderedAccessView(m_raytracingOutput_ao.Get(), nullptr, &UAVDesc,
    // m_srvUavHeap->GetCpuHandle(SrvUavDescriptors::AmbientMap0Uav));
    //device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
    //m_raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(
    // m_srvUavHeap->GetGpuHandle(SrvUavDescriptors::DxrUav));
}

void Game::CreateGeometryAndMaterialResources(ID3D12Device* device)
//void Game::BuildGeometryAndMaterials(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
    //const auto device = m_deviceResources->GetD3DDevice();
    const auto commandQueue    = m_deviceResources->GetCommandQueue();
    const auto backBufferCount = m_deviceResources->GetBackBufferCount();

    // All geometry is rendered for a right-handed coordinate system.
    // Legend - x { l: left, r: right }; y { t: top, b: bottom }; z { f: front, k: back }
    //        - n { u: up, d: down, l: left, r: right, i: into screen, o: out of screen }

    // Create geometric primitives.
    GeometricPrimitive::VertexCollection geoSphereVertices;
    GeometricPrimitive::IndexCollection  geoSphereIndices;
    m_geospherePrimitive = GeometricPrimitive::CreateGeoSphere();
    GeometricPrimitive::CreateGeoSphere(geoSphereVertices, geoSphereIndices);

    // Cube.
    const std::vector<VertexPositionNormalTexture> cubeVertices =
    {
        //{ Vector3( 0.00f, +0.25f * m_aspectRatio, 0), Vector4(1, 1, 0, 1) }, // yellow
        //{ Vector3(+0.25f, -0.25f * m_aspectRatio, 0), Vector4(0, 1, 1, 1) }, // cyan
        //{ Vector3(-0.25f, -0.25f * m_aspectRatio, 0), Vector4(1, 0, 1, 1) }, // magenta
        // Top face.
        { Vector3(-0.1f,+0.1f,-0.1f), Vector3( 0,+1, 0), Vector2(0,0) }, // 0 ltk u
        { Vector3(+0.1f,+0.1f,-0.1f), Vector3( 0,+1, 0), Vector2(1,0) }, // 1 rtk u
        { Vector3(+0.1f,+0.1f,+0.1f), Vector3( 0,+1, 0), Vector2(1,1) }, // 2 rtf u
        { Vector3(-0.1f,+0.1f,+0.1f), Vector3( 0,+1, 0), Vector2(0,1) }, // 3 ltf u
        // Left face.
        { Vector3(-0.1f,+0.1f,-0.1f), Vector3(-1, 0, 0), Vector2(0,0) }, // 4 ltk l
        { Vector3(-0.1f,+0.1f,+0.1f), Vector3(-1, 0, 0), Vector2(1,0) }, // 5 ltf l
        { Vector3(-0.1f,-0.1f,+0.1f), Vector3(-1, 0, 0), Vector2(1,1) }, // 6 lbf l
        { Vector3(-0.1f,-0.1f,-0.1f), Vector3(-1, 0, 0), Vector2(0,1) }, // 7 lbk l
        // Front face.
        { Vector3(-0.1f,+0.1f,+0.1f), Vector3( 0, 0,+1), Vector2(0,0) }, // 8 ltf o
        { Vector3(+0.1f,+0.1f,+0.1f), Vector3( 0, 0,+1), Vector2(1,0) }, // 9 rtf o
        { Vector3(+0.1f,-0.1f,+0.1f), Vector3( 0, 0,+1), Vector2(1,1) }, //10 rbf o
        { Vector3(-0.1f,-0.1f,+0.1f), Vector3( 0, 0,+1), Vector2(0,1) }, //11 lbf o
        // Right face.
        { Vector3(+0.1f,+0.1f,+0.1f), Vector3(+1, 0, 0), Vector2(0,0) }, //12 rtf r
        { Vector3(+0.1f,+0.1f,-0.1f), Vector3(+1, 0, 0), Vector2(1,0) }, //13 rtk r
        { Vector3(+0.1f,-0.1f,-0.1f), Vector3(+1, 0, 0), Vector2(1,1) }, //14 rbk r
        { Vector3(+0.1f,-0.1f,+0.1f), Vector3(+1, 0, 0), Vector2(0,1) }, //15 rbf r
        // Back face.
        { Vector3(+0.1f,+0.1f,-0.1f), Vector3( 0, 0,-1), Vector2(0,0) }, //16 rtk i
        { Vector3(-0.1f,+0.1f,-0.1f), Vector3( 0, 0,-1), Vector2(1,0) }, //17 ltk i
        { Vector3(-0.1f,-0.1f,-0.1f), Vector3( 0, 0,-1), Vector2(1,1) }, //18 lbk i
        { Vector3(+0.1f,-0.1f,-0.1f), Vector3( 0, 0,-1), Vector2(0,1) }, //19 rbk i
        // Bottom face.
        { Vector3(-0.1f,-0.1f,+0.1f), Vector3( 0,-1, 0), Vector2(0,0) }, //20 lbf d
        { Vector3(+0.1f,-0.1f,+0.1f), Vector3( 0,-1, 0), Vector2(1,0) }, //21 rbf d
        { Vector3(+0.1f,-0.1f,-0.1f), Vector3( 0,-1, 0), Vector2(1,1) }, //22 rbk d
        { Vector3(-0.1f,-0.1f,-0.1f), Vector3( 0,-1, 0), Vector2(0,1) }, //23 lbk d
    };

    // Testing geometry with _r32 index format.
    const std::vector<uint32_t> cubeIndices =
    //const std::vector<uint16_t> cubeIndices =
    {   // Clockwise winding.
        // If rendering a single triangle only, add a dummy index so the index buffer srv is created with
        // two uint32 elements (enough to fit all three indices).
        //0, 1, 2, 0,
        //0, 2, 1, 0,
        // Top face.
        0,1,2,
        0,2,3,
        // Left face.
        4,5,6,
        4,6,7,
        // Front face.
        8,9,10,
        8,10,11,
        // Right face.
        12,13,14,
        12,14,15,
        // Back face.
        16,17,18,
        16,18,19,
        // Bottom face.
        20,21,22,
        20,22,23,
    };

    // Ground plane geometry.

    //const std::vector<VertexPositionNormalTexture> planeVertices =
    ////const std::vector<VertexPositionNormal> planeVertices =
    //{
    //    { Vector3(-1.f,-0.15f,-1.f), Vector3( 0,+1, 0), Vector2(0,0) }, // 0 lk u
    //    { Vector3(+1.f,-0.15f,-1.f), Vector3( 0,+1, 0), Vector2(1,0) }, // 1 rk u
    //    { Vector3(+1.f,-0.15f,+1.f), Vector3( 0,+1, 0), Vector2(1,1) }, // 2 rf u
    //    { Vector3(-1.f,-0.15f,+1.f), Vector3( 0,+1, 0), Vector2(0,1) }, // 3 lf u
    //};
    //
    //const std::vector<uint16_t> planeIndices =
    //{   // Clockwise winding.
    //    0, 1, 2,
    //    0, 2, 3,
    //    //3, 1, 0,
    //    //2, 1, 3,
    //};

    // Smart pointers can be passed to functions by reference.
    auto LoadStaticModel = [&](std::unique_ptr<SDKMESHModel>& _model, const wchar_t* _renderingFile, const wchar_t* _collisionFile)
        {
            _model = std::make_unique<SDKMESHModel>(device, commandQueue, _renderingFile, _collisionFile);
        };
    auto LoadSkinnedModel = [&](std::unique_ptr<FBXModel>& _model, const char* _renderingFile)
        {
            _model = std::make_unique<FBXModel>(device, commandQueue, _renderingFile);
        };

    // Loading models with multithreading requires passing smart pointer with std::ref.
    // Load FBX models on a separate thread batch since they have a longer load time.
    std::vector<std::thread> sdkMeshThreads;
    std::vector<std::thread> fbxThreads;

    sdkMeshThreads.push_back(std::thread(LoadStaticModel, std::ref(m_SDKMESHModel[SDKMESHModels::Suzanne]), L"Models\\Suzanne.sdkmesh", L"Models\\Suzanne.sdkmesh"));
    sdkMeshThreads.push_back(std::thread(LoadStaticModel, std::ref(m_SDKMESHModel[SDKMESHModels::Palmtree]), L"Models\\Palmtree.sdkmesh", L"Models\\Palmtree.sdkmesh"));
    sdkMeshThreads.push_back(std::thread(LoadStaticModel, std::ref(m_SDKMESHModel[SDKMESHModels::MiniRacecar]), L"Models\\MiniRaceCar.sdkmesh", L"Models\\MiniRaceCar.sdkmesh"));
    sdkMeshThreads.push_back(std::thread(LoadStaticModel, std::ref(m_SDKMESHModel[SDKMESHModels::Racetrack]), L"Models\\AlbertParkAll.sdkmesh", L"Models\\AlbertParkAll.sdkmesh"));
    //m_SDKMESHModel[SDKMESHModels::Suzanne]      = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\Suzanne.sdkmesh", L"Models\\Suzanne.sdkmesh");
    //m_SDKMESHModel[SDKMESHModels::Racetrack]    = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\RacetrackTwoMesh.sdkmesh", L"Models\\RacetrackCollision.sdkmesh");
    //m_SDKMESHModel[SDKMESHModels::Palmtree]     = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\Palmtree.sdkmesh", L"Models\\Palmtree.sdkmesh");
    //m_SDKMESHModel[SDKMESHModels::MiniRacecar]  = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\MiniRaceCar.sdkmesh", L"Models\\MiniRaceCar.sdkmesh");
    //m_SDKMESHModel[SDKMESHModels::Racetrack]    = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\AlbertParkAll.sdkmesh", L"Models\\AlbertParkAll.sdkmesh");
    //m_racetrack   = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\RacetrackTwoMesh.sdkmesh", L"Models\\RacetrackCollision.sdkmesh");
    //m_palmtree    = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\Palmtree.sdkmesh", L"Models\\Palmtree.sdkmesh");
    //m_miniracecar = std::make_unique<SDKMESHModel>(device, commandQueue, L"Models\\MiniRaceCar.sdkmesh", L"Models\\MiniRaceCar.sdkmesh");
    //m_racetrack = std::make_unique<StaticModel>(device, commandQueue, L"Models\\racetrack.sdkmesh", L"Models\\racetrack.sdkmesh");
    //m_racetrack_grass = std::make_unique<StaticModel>(device, commandQueue, L"Models\\racetrack_grass.sdkmesh", L"Models\\racetrack_grass.sdkmesh");
    //m_racetrack_road = std::make_unique<StaticModel>(device, commandQueue, L"Models\\racetrack_road.sdkmesh", L"Models\\racetrack_road.sdkmesh");
    //m_suzanne = std::make_unique<StaticModel>(device,
    // L"Models\\suzanneFBX.sdkmesh", L"Models\\suzanneFBX.sdkmesh");
    //assert(m_palmtree_trunk->GetIndexFormat(0,0) == DXGI_FORMAT_R32_UINT);
    //assert(m_palmtree_trunk->GetIndexStart(0,0)==0);

    // Load FBX models.
    fbxThreads.push_back(std::thread(LoadSkinnedModel, std::ref(m_FBXModel[FBXModels::Dove]), "Models\\Dove.fbx"));
    //m_FBXModel[FBXModels::Dove] = std::make_unique<FBXModel>(device, commandQueue, "Models\\Dove.fbx");
    //m_dove = std::make_unique<FBXModel>(device, commandQueue, "Models\\Dove.fbx");

    // Create command buffer for indirect drawing.
    std::vector<IndirectCommand> commands;

    commands.resize(backBufferCount);
    m_constantBufferIndirect->Create(device, backBufferCount, L"ConstBufferIndirect"); // Resource name required.

    for (uint32_t frame = 0; frame < backBufferCount; frame++)
    {
        const auto gpuAddress = m_constantBufferIndirect->GpuVirtualAddress(frame);

        commands[frame].cbv = gpuAddress;
        commands[frame].drawIndexedArguments.IndexCountPerInstance = static_cast<uint32_t>(geoSphereIndices.size());
        //commands[frame].drawArguments.VertexCountPerInstance = static_cast<uint32_t>(geoSphereVertices.size());
        commands[frame].drawIndexedArguments.InstanceCount = 1;
        commands[frame].drawIndexedArguments.BaseVertexLocation = 0;
        //commands[frame].drawIndexedArguments StartVertexLocation = 0;
        commands[frame].drawIndexedArguments.StartIndexLocation = 0;
        commands[frame].drawIndexedArguments.StartInstanceLocation = 0;
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_geospherePrimitive->LoadStaticBuffers(device, resourceUpload);
    // Get a copy of the geosphere geometry buffers for indirect drawing.
    auto& geoSphereBuffers = m_procGeometry[ProcGeometries::GeoSphere];
    ThrowIfFailed(CreateStaticBuffer(
        device,
        resourceUpload,
        geoSphereVertices,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        &geoSphereBuffers.vertexBuffer
    ));
    ThrowIfFailed(CreateStaticBuffer(
        device,
        resourceUpload,
        geoSphereIndices,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        &geoSphereBuffers.indexBuffer
    ));
    auto& cubeBuffers = m_procGeometry[ProcGeometries::Cube];
    ThrowIfFailed(CreateStaticBuffer(
        device,
        resourceUpload,
        cubeVertices,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        &cubeBuffers.vertexBuffer
    ));
    ThrowIfFailed(CreateStaticBuffer(
        device,
        resourceUpload,
        cubeIndices,
        D3D12_RESOURCE_STATE_INDEX_BUFFER,
        &cubeBuffers.indexBuffer
    ));

    //auto& planeBuffers = m_procGeometry[GeometryInstances::Plane];
    //ThrowIfFailed(CreateStaticBuffer(
    //    device,
    //    resourceUpload,
    //    planeVertices,
    //    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    //    &planeBuffers.vertexBuffer
    //));
    //ThrowIfFailed(CreateStaticBuffer(
    //    device,
    //    resourceUpload,
    //    planeIndices,
    //    D3D12_RESOURCE_STATE_INDEX_BUFFER,
    //    &planeBuffers.indexBuffer
    //));

    // Vertex/index buffer creation for static model.
    //auto const renderModel = m_suzanne->GetRenderingModel();
    //renderModel->LoadStaticBuffers(device, resourceUpload, true);
    //m_suzanne->GetModelData(*renderModel);
    //m_suzanne->GetCollisionModel()->LoadStaticBuffers(device, resourceUpload, true);
    //m_suzanne->LoadMeshOnCPU();

    // Upload FBX models.
    //for (auto& mesh : *m_dove.get())
    //{
    //    ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, mesh.finalVertices,
    // D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &mesh.vertexBuffer));
    //    ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, mesh.finalIndices,
    // D3D12_RESOURCE_STATE_INDEX_BUFFER, &mesh.indexBuffer));
    //    m_dove->CreateBufferViews(mesh);
    //}

    // Don't create aliases for unfilled pointers... this also leaves live objects at program exit.
    //auto rtaoBuffer = m_structBuffer[StructuredBuffers::RtaoInstData].Get();
    //auto colorBuffer = m_structBuffer[StructuredBuffers::ColorInstData].Get();

    //ThrowIfFailed(CreateStaticBuffer(
    //    device,
    //    resourceUpload,
    //    geometryData, //objectDataRtao,
    //    D3D12_RESOURCE_STATE_GENERIC_READ,
    //    &m_structBuffer[StructBuffers::GeometryData]
    //));
    // Upload the command buffer(s) for indirect drawing.
    ThrowIfFailed(CreateStaticBuffer(
        device,
        resourceUpload,
        commands,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        &m_commandBuffer
    ));

    //ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, vertices,
    // D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexBuffer.GetAddressOf()));
    //ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, indices,
    // D3D12_RESOURCE_STATE_INDEX_BUFFER, m_indexBuffer.GetAddressOf()));
    //
    //ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, planeVertices,
    // D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_planeVertexBuffer.GetAddressOf()));
    //ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, planeIndices,
    // D3D12_RESOURCE_STATE_INDEX_BUFFER, m_planeIndexBuffer.GetAddressOf()));

    // Initialize texture factory and load textures.
    m_texFactory = std::make_unique<EffectTextureFactory>(
        device,
        resourceUpload,
        m_descHeap[DescriptorHeaps::SrvUav]->Heap()
    );
    m_texFactory->SetDirectory(L".\\Textures");

    auto LoadTexture = [&](const wchar_t* _ddsFile, int _descriptorIndex)
        {
            m_texFactory->CreateTexture(_ddsFile, _descriptorIndex);
        };

    LoadTexture(L"Grass_1K_Cube.dds", SrvUAVs::EnvironmentMapSrv);
    LoadTexture(L"Suzanne_1K_BaseColor.dds", SrvUAVs::SuzanneAlbedoSrv);
    LoadTexture(L"Suzanne_1K_Normal.dds", SrvUAVs::SuzanneNormalSrv);
    LoadTexture(L"Grass01_2K_BaseColor.dds", SrvUAVs::RacetrackSkirtAlbedoSrv);
    LoadTexture(L"Grass01_2K_Normal.dds", SrvUAVs::RacetrackSkirtNormalSrv);
    LoadTexture(L"Grass01_2K_RMA.dds", SrvUAVs::RacetrackSkirtRMASrv);
    LoadTexture(L"SingleLaneRoadClean01_4K_BaseColor.dds", SrvUAVs::RacetrackRoadAlbedoSrv);
    LoadTexture(L"SingleLaneRoadClean01_4K_Normal.dds", SrvUAVs::RacetrackRoadNormalSrv);
    LoadTexture(L"SingleLaneRoadClean01_4K_RMA.dds", SrvUAVs::RacetrackRoadRMASrv);
    LoadTexture(L"AlbertParkMap_2K_BaseColor.dds", SrvUAVs::RacetrackMapAlbedoSrv);
    LoadTexture(L"Palmtree_2K_BaseColor_aNonPM.dds", SrvUAVs::PalmtreeAlbedoSrv);
    LoadTexture(L"Palmtree_2K_Normal.dds", SrvUAVs::PalmtreeNormalSrv);
    LoadTexture(L"Palmtree_2K_RMA.dds", SrvUAVs::PalmtreeRMASrv);
    LoadTexture(L"MiniRaceCar_2K_BaseColor.dds", SrvUAVs::MiniRacecarAlbedoSrv);
    LoadTexture(L"MiniRaceCar_2K_RMA.dds", SrvUAVs::MiniRacecarRMASrv);
    LoadTexture(L"Dove_2K_BaseColor.dds", SrvUAVs::DoveAlbedoSrv);
    LoadTexture(L"Dove_2K_Normal.dds", SrvUAVs::DoveNormalSrv);
    LoadTexture(L"Sphere2Mat_BaseColor.dds", SrvUAVs::CubeAlbedoSrv);
    LoadTexture(L"Sphere2Mat_Normal.dds", SrvUAVs::CubeNormalSrv);
    LoadTexture(L"Sphere2Mat_OcclusionRoughnessMetallic.dds", SrvUAVs::CubeRMASrv);
    LoadTexture(L"Sphere2Mat_Emissive.dds", SrvUAVs::CubeEmissiveSrv);
    LoadTexture(L"Default_2K_Normal.dds", SrvUAVs::DefaultNormalSrv);
    LoadTexture(L"Default_2K_RMA.dds", SrvUAVs::DefaultRMASrv);
    //m_texFactory->CreateTexture(L"SunSubMixer_diffuseIBL.dds",               SrvUAVs::DiffuseIBLSrv);
    //m_texFactory->CreateTexture(L"SunSubMixer_specularIBL.dds",              SrvUAVs::SpecularIBLSrv);

    auto uploadResourcesFinished = resourceUpload.End(commandQueue);
    uploadResourcesFinished.wait();

    // Create SRVs for vertex, index and structured buffers.
    // We are replacing the original sample CreateBufferSRV method with DXTK & custom methods.

    // Cube
    CreateBufferShaderResourceView(
        device,
        cubeBuffers.vertexBuffer.Get(),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::CubeVertexBufferSrv),
        sizeof(VertexPositionNormalTexture)
    );
    CreateByteAddressShaderResourceView(
        device,
        cubeBuffers.indexBuffer.Get(),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::CubeIndexBufferSrv)
    );

    // Ground plane
    //CreateBufferShaderResourceView(
    //    device,
    //    planeBuffers.vertexBuffer.Get(),
    //    m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::PlaneVertexBufferSrv),
    //    sizeof(VertexPositionNormalTexture)
    //);
    //CreateByteAddressShaderResourceView(
    //    device,
    //    planeBuffers.indexBuffer.Get(),
    //    m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::PlaneIndexBufferSrv)
    //);

    // Wait for SDKMESH model loading threads to complete.
    for (auto& t : sdkMeshThreads)
        t.join();

    // Suzanne
    auto sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Suzanne].get();

    CreateBufferShaderResourceView(
        device,
        sdkMeshModel->GetVertexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::SuzanneVertexBufferSrv),
        sdkMeshModel->GetVertexStride(0,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        sdkMeshModel->GetIndexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::SuzanneIndexBufferSrv)
    );

    // Racetrack road
    sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Racetrack].get();  // This cost me a day of merged BLAS debugging!

    CreateBufferShaderResourceView(
        device,
        //m_racetrack_road->GetVertexBuffer(0,0),
        sdkMeshModel->GetVertexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::RacetrackRoadVertexBufferSrv),
        //m_racetrack_road->GetVertexStride(0,0)
        sdkMeshModel->GetVertexStride(0,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        //m_racetrack_road->GetIndexBuffer(0,0),
        sdkMeshModel->GetIndexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::RacetrackRoadIndexBufferSrv)
    );

    // Racetrack grass
    CreateBufferShaderResourceView(
        device,
        //m_racetrack_grass->GetVertexBuffer(0,0),
        sdkMeshModel->GetVertexBuffer(1,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::RacetrackSkirtVertexBufferSrv),
        //m_racetrack_grass->GetVertexStride(0,0)
        sdkMeshModel->GetVertexStride(1,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        //m_racetrack_grass->GetIndexBuffer(0,0),
        sdkMeshModel->GetIndexBuffer(1,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::RacetrackSkirtIndexBufferSrv)
    );

    // Racetrack map
    CreateBufferShaderResourceView(
        device,
        sdkMeshModel->GetVertexBuffer(2,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::RacetrackMapVertexBufferSrv),
        sdkMeshModel->GetVertexStride(2,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        sdkMeshModel->GetIndexBuffer(2,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::RacetrackMapIndexBufferSrv)
    );

    // Palmtree trunk
    sdkMeshModel = m_SDKMESHModel[SDKMESHModels::Palmtree].get();

    CreateBufferShaderResourceView(
        device,
        sdkMeshModel->GetVertexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::PalmtreeTrunkVertexBufferSrv),
        sdkMeshModel->GetVertexStride(0,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        sdkMeshModel->GetIndexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::PalmtreeTrunkIndexBufferSrv)
    );

    // Palmtree canopy
    CreateBufferShaderResourceView(
        device,
        sdkMeshModel->GetVertexBuffer(1,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::PalmtreeCanopyVertexBufferSrv),
        sdkMeshModel->GetVertexStride(1,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        sdkMeshModel->GetIndexBuffer(1,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::PalmtreeCanopyIndexBufferSrv)
    );

    // Mini racecar
    sdkMeshModel = m_SDKMESHModel[SDKMESHModels::MiniRacecar].get();

    CreateBufferShaderResourceView(
        device,
        sdkMeshModel->GetVertexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::MiniRacecarVertexBufferSrv),
        sdkMeshModel->GetVertexStride(0,0)
    );
    CreateByteAddressShaderResourceView(
        device,
        sdkMeshModel->GetIndexBuffer(0,0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::MiniRacecarIndexBufferSrv)
    );

    // Wait for FBX model loading threads to complete.
    for (auto& t : fbxThreads)
        t.join();

    // Dove
    auto fbxModel = m_FBXModel[FBXModels::Dove].get();

    CreateBufferShaderResourceView(
        device,
        fbxModel->GetVertexBuffer(0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::DoveVertexBufferSrv),
        fbxModel->GetVertexStride()
    );
    CreateBufferUnorderedAccessView(
        device,
        fbxModel->GetSkinnedVertexBuffer(0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::DoveSkinnedVertexBufferUav),
        fbxModel->GetSkinnedVertexStride()
    );
    CreateByteAddressShaderResourceView(
        device,
        fbxModel->GetIndexBuffer(0),
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::DoveIndexBufferSrv)
    );

    //CreateBufferSRV(cubeBuffers.vertexBuffer.Get(), static_cast<uint32_t>(cubeVertices.size()),
    // sizeof(VertexPositionNormal), SrvUavDescriptors::CubeVertexBufferSrv);
    //CreateBufferSRV(planeBuffers.vertexBuffer.Get(), static_cast<uint32_t>(planeVertices.size()),
    // sizeof(VertexPositionNormalTexture), SrvUavDescriptors::PlaneVertexBufferSrv);
    //CreateBufferSRV(m_suzanne->GetVertexBuffer(), m_suzanne->GetNumVertices(),
    // m_suzanne->GetVertexStride(), SrvUavDescriptors::SuzanneVertexBufferSrv);

    //CreateBufferSRV(cubeBuffers.indexBuffer.Get(), static_cast<uint32_t>(cubeIndices.size()) / 2, 0,
    // SrvUavDescriptors::CubeIndexBufferSrv); // express number of indices in units of uint32
    //CreateBufferSRV(planeBuffers.indexBuffer.Get(), static_cast<uint32_t>(planeIndices.size()) / 2, 0,
    // SrvUavDescriptors::PlaneIndexBufferSrv); // express number of indices in units of uint32
    //CreateBufferSRV(m_suzanne->GetIndexBuffer(), m_suzanne->GetNumIndices() / 2, 0,
    // SrvUavDescriptors::SuzanneIndexBufferSrv); // express number of indices in units of uint32

    // Structured buffers.
    //CreateBufferShaderResourceView(
    //    device,
    //    m_structBuffer[StructBuffers::GeometryData].Get(),
    //    m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::GeometryBufferSrv),
    //    sizeof(MeshGeometryData)
    //    //sizeof(RtaoObjectData)
    //);

    // Create SRVs per backbuffer for indirect drawing buffers.
    const auto constantBuffer = m_constantBufferIndirect->GetResource();
    CreateBufferShaderResourceViewPerFrameIndex(
        device,
        constantBuffer,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::FrameConstantsSrv_0),
        1, Align(sizeof(FrameConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), 0
    );
    CreateBufferShaderResourceViewPerFrameIndex(
        device,
        constantBuffer,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::FrameConstantsSrv_1),
        1, Align(sizeof(FrameConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), 1
    );

    const auto commandBuffer = m_commandBuffer.Get();
    CreateBufferShaderResourceViewPerFrameIndex(
        device,
        commandBuffer,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::CommandBufferSrv_0),
        1, sizeof(IndirectCommand), 0
    );
    CreateBufferShaderResourceViewPerFrameIndex(
        device,
        commandBuffer,
        m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::CommandBufferSrv_1),
        1, sizeof(IndirectCommand), 1
    );

    // Fill the vertex buffer view & index buffer view for the geosphere (required for indirect drawing).
    geoSphereBuffers.vertexBufferView.BufferLocation = geoSphereBuffers.vertexBuffer->GetGPUVirtualAddress();
    geoSphereBuffers.vertexBufferView.StrideInBytes = sizeof(VertexPositionNormalTexture);
    geoSphereBuffers.vertexBufferView.SizeInBytes =
        static_cast<uint32_t>(geoSphereVertices.size()) * sizeof(VertexPositionNormalTexture);

    auto indexCountPerInstance = static_cast<uint32_t>(geoSphereIndices.size());
    geoSphereBuffers.indexBufferView.BufferLocation = geoSphereBuffers.indexBuffer->GetGPUVirtualAddress();
    geoSphereBuffers.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    geoSphereBuffers.indexBufferView.SizeInBytes =indexCountPerInstance * sizeof(uint16_t);
    geoSphereBuffers.indexCountPerInstance = indexCountPerInstance;

    // Fill the vertex buffer view & index buffer view for custom geometries.
    cubeBuffers.vertexBufferView.BufferLocation = cubeBuffers.vertexBuffer->GetGPUVirtualAddress();
    cubeBuffers.vertexBufferView.StrideInBytes  = sizeof(VertexPositionNormalTexture);
    cubeBuffers.vertexBufferView.SizeInBytes =
        static_cast<uint32_t>(cubeVertices.size()) * sizeof(VertexPositionNormalTexture);

    indexCountPerInstance = static_cast<uint32_t>(cubeIndices.size());
    cubeBuffers.indexBufferView.BufferLocation = cubeBuffers.indexBuffer->GetGPUVirtualAddress();
    cubeBuffers.indexBufferView.Format = DXGI_FORMAT_R32_UINT;// DXGI_FORMAT_R16_UINT;
    cubeBuffers.indexBufferView.SizeInBytes = indexCountPerInstance * sizeof(uint32_t);// sizeof(uint16_t);
    cubeBuffers.indexCountPerInstance = indexCountPerInstance;
    // Transform instance buffer view size is scene dependent (cubes per scene may vary), so set before the scene draw call.
    //cubeBuffers.transformInstanceBufferView.SizeInBytes = SceneMain::CubeInstanceCount * sizeof(XMFLOAT3X4);
    //cubeBuffers.transformInstanceBufferView.SizeInBytes = Globals::CubeInstanceCount * sizeof(XMFLOAT3X4);
    cubeBuffers.transformInstanceBufferView.StrideInBytes = sizeof(XMFLOAT3X4);

    //planeBuffers.vertexBufferView.BufferLocation = planeBuffers.vertexBuffer->GetGPUVirtualAddress();
    //planeBuffers.vertexBufferView.StrideInBytes  = sizeof(VertexPositionNormalTexture);
    //planeBuffers.vertexBufferView.SizeInBytes =
    //    static_cast<uint32_t>(planeVertices.size()) * sizeof(VertexPositionNormalTexture);
    ////m_planeVertexBufferView.StrideInBytes = sizeof(VertexPositionNormal);
    ////m_planeVertexBufferView.SizeInBytes = static_cast<UINT>(planeVertices.size()) * sizeof(VertexPositionNormal);
    //
    //planeBuffers.indexBufferView.BufferLocation = planeBuffers.indexBuffer->GetGPUVirtualAddress();
    //planeBuffers.indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    //planeBuffers.indexBufferView.SizeInBytes =
    //    static_cast<uint32_t>(planeIndices.size()) * sizeof(uint16_t);
    
    /*
    // Initialize cube transforms.
    // Instance #0
    auto transform = Matrix::CreateTranslation(Vector3(0, 0.15f, 0));// Matrix::Identity;
    //m_cubeTransforms[0] = transform;
    XMStoreFloat3x4(&m_cubeTransforms[0], transform);
    //// Instance #1
    transform = Matrix::CreateTranslation(Vector3(-6, 0.15f, 0));
    //m_cubeTransforms[1] = transform;
    XMStoreFloat3x4(&m_cubeTransforms[1], transform);
    //// Instance #2
    transform = Matrix::CreateTranslation(Vector3(6, 0.15f, 0));
    //m_cubeTransforms[2] = transform;
    XMStoreFloat3x4(&m_cubeTransforms[2], transform);
    */

    // Set constants for cubes.
    //m_cubeConstants[0].material.albedo = Vector3(1, 0, 0); // red
    //m_cubeConstants[1].material.albedo = Vector3(0, 1, 0); // green
    //m_cubeConstants[2].material.albedo = Vector3(0, 0, 1); // blue
    //m_cubeConstants[0].albedo = Vector4(1, 0, 0, 1); // red
    //m_cubeConstants[1].albedo = Vector4(0, 1, 0, 1); // green
    //m_cubeConstants[2].albedo = Vector4(0, 0, 1, 1); // blue


    // Edit: We will now access color instance data from a structured buffer within the pixel shader.
    // Get diffuse colors for rasterized instanced cubes.
    //Vector4 cubeColors[3] = {};
    ////for (UINT i = 0; i < CUBE_INSTANCE_COUNT; ++i)
    //for (uint32_t i = 0; i < Globals::CubeInstanceCount; ++i)
    //{
    //    cubeColors[i] = objectDataColor[i].material.albedo;
    //    //m_cubeConstants[i].vbIndex = SrvUavDescriptors::CubeVertexBufferSrv;
    //    //cubeColors[i] = m_cubeConstants[i].material.albedo;
    //}

    // Set constants for plane.
    //*m_planeConstants =
    //{
    //    0.25f,  // reflectanceCoef
    //    1.f,    // diffuseCoef
    //    0.4f,   // specularCoef
    //    50.f,   // specularPower
    //    Vector3::One, // albedo
    //    SrvUavDescriptors::PlaneVertexBufferSrv, // vertex buffer index
    //    //Vector4::One, // albedo
    //};

    //uint32_t colorInstanceSize = Globals::Cube_Instance_Count * sizeof(Vector3);
    //uint32_t materialInstanceSize = Globals::Cube_Instance_Count * sizeof(ObjectConstants);
    //UINT materialInstanceSize = m_geometryInstanceCount * sizeof(Vector4);

    //auto cubeColorsMem = m_graphicsMemory->AllocateConstant(cubeColors);
    //auto res2 = m_graphicsMemory->Allocate(colorInstanceSize);
    //memcpy(res2.Memory(), cubeColors, colorInstanceSize);

    //GraphicsResource res2 = m_graphicsMemory->Allocate(materialInstanceSize);
    //GraphicsResource res2 = GraphicsMemory::Get().Allocate(materialInstanceSize);
    //memcpy(res2.Memory(), m_cubeConstants.get(), materialInstanceSize);

    // Edit: We are no longer using a vertex buffer view for material instances.
    //cubeBuffers.materialInstanceBufferView.BufferLocation = cubeColorsMem.GpuAddress();
    //cubeBuffers.materialInstanceBufferView.SizeInBytes    = Globals::CubeInstanceCount * sizeof(Vector4);
    //cubeBuffers.materialInstanceBufferView.StrideInBytes  = sizeof(Vector4);
    //m_materialInstanceBufferView.SizeInBytes = materialInstanceSize;
    //m_materialInstanceBufferView.StrideInBytes = sizeof(ObjectConstants);
    //m_materialInstanceBufferView.StrideInBytes = sizeof(Vector4);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    // If using the DirectX Tool Kit for DX12, uncomment this line:
    //m_graphicsMemory.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
    //CreateRaytracingResources();
}
#pragma endregion

void Game::DoRaytracing(
    ID3D12GraphicsCommandList4* commandList,
    ID3D12StateObject* stateObject,
    const ShaderBindingTableGroup* sbtGroup,
    int width, int height)
{
    //auto commandList = m_deviceResources->GetCommandList();

    auto DispatchRays = [&](auto* _dispatchDesc)
        //auto DispatchRays = [&](auto* _commandList, auto* _stateObject, auto* _dispatchDesc)
        {
            // Since each shader table has only one shader record, the stride is same as the size.
            _dispatchDesc->HitGroupTable.StartAddress = sbtGroup->hitgroup.buffer->GetGPUVirtualAddress();
            _dispatchDesc->HitGroupTable.SizeInBytes = sbtGroup->hitgroup.buffer->GetDesc().Width;
            //dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->buffer->GetGPUVirtualAddress();
            //dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->buffer->GetDesc().Width;
            // Instances increase the stride, so cannot use dispatchDesc->HitGroupTable.SizeInBytes
            _dispatchDesc->HitGroupTable.StrideInBytes = sbtGroup->hitgroup.strideInBytes;
            //dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
            _dispatchDesc->MissShaderTable.StartAddress = sbtGroup->miss.buffer->GetGPUVirtualAddress();
            _dispatchDesc->MissShaderTable.SizeInBytes = sbtGroup->miss.buffer->GetDesc().Width;
            _dispatchDesc->MissShaderTable.StrideInBytes = sbtGroup->miss.strideInBytes;
            //dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
            //dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
            //dispatchDesc->MissShaderTable.StrideInBytes = m_missShaderTableStrideInBytes;
            //dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
            _dispatchDesc->RayGenerationShaderRecord.StartAddress = sbtGroup->raygeneration.buffer->GetGPUVirtualAddress();
            _dispatchDesc->RayGenerationShaderRecord.SizeInBytes = sbtGroup->raygeneration.buffer->GetDesc().Width;
            _dispatchDesc->Width = width;
            _dispatchDesc->Height = height;
            _dispatchDesc->Depth = 1;
            commandList->SetPipelineState1(stateObject);
            commandList->DispatchRays(_dispatchDesc);
        };

    // Bind the heaps and all resources required by the shader, and dispatch rays.
    // MUST be called before Set*RootSignature if using HLSL dynamic resources.
    //ID3D12DescriptorHeap* heap[] = { m_descHeap[DescriptorHeaps::SrvUav]->Heap()};
    //commandList->SetDescriptorHeaps(1, heap);

    // Important: The global compute signature has already been set, so we do not need to set again.
    // A different rootsig was set for denoising the ao scene, so reset the global raytracing rootsig
    // and rebind all root parameter resources.
    //commandList->SetComputeRootSignature(m_rootSig[RootSignatures::DxrGlobal].Get());

    // Set output, vertex & index buffer descriptor tables, and TLAS srv.
    //commandList->SetComputeRootDescriptorTable(GlobalRootSigParams::OutputViewSlot, m_srvUavHeap->GetGpuHandle(SrvUavDescriptors::DxrUav));
    //commandList->SetComputeRootDescriptorTable(GlobalRootSigParams::VertexBufferSlot, m_srvUavHeap->GetGpuHandle(SrvUavDescriptors::VertexBufferSrv));
    //commandList->SetComputeRootShaderResourceView(GlobalRootSigParams::AccelerationStructureSlot, m_topLevelAS->GetGPUVirtualAddress());

    const auto frameConstants = m_scene->GetFrameConstants();

    const auto frameCB = m_graphicsMemory->AllocateConstant(*frameConstants);
    commandList->SetComputeRootConstantBufferView(ComputeRootSigParams::FrameCB, frameCB.GpuAddress());
    //commandList->SetComputeRootShaderResourceView(
    //    ComputeRootSigParams::AccelerationStructure, m_tlasBuffers->accelerationStructure->GetGPUVirtualAddress()
    //    //GlobalRootSigParams::AccelerationStructure, m_tlasBuffers->accelerationStructure->GetGPUVirtualAddress()
    //);

    // Set camera constants on the gpu.
    //auto cameraCB = m_graphicsMemory->AllocateConstant(*m_frameConstants);
    //commandList->SetComputeRootConstantBufferView(GlobalRootSigParams::CameraConstants, cameraCB.GpuAddress());

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    DispatchRays(&dispatchDesc);
    //DispatchRays(commandList, stateObject, &dispatchDesc);
    //DispatchRays(m_dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc);
}

// Copy the raytracing output to the backbuffer.
// Edit: We now copy to both a HDR render target and a history buffer.
void Game::CopyRaytracingOutputToBackbuffer(ID3D12GraphicsCommandList7* commandList)
{
    //auto commandList = m_deviceResources->GetCommandList();

    const auto renderTarget = m_renderTex[RenderTextures::Hdr]->GetResource();
    const auto raytracingOutput = m_raytracingOutput.Get();
    //auto renderTarget = m_hdrTex->GetResource();
    //auto renderTarget = m_deviceResources->GetRenderTarget();
    //auto ambientMap = m_ssao->AmbientMap();
    
    D3D12_RESOURCE_BARRIER preCopyBarriers[1] = {};
    preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    //preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
    //    m_raytracingOutput.Get(),
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    D3D12_RESOURCE_STATE_COPY_SOURCE
    //);
    //preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
    // ambientMap, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(1, preCopyBarriers);

    // State transition the raytracing output resource using Enhanced Barriers.
    D3D12_TEXTURE_BARRIER barrierUnorderedAccessToCopySource[] =
    {
        CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_ALL_SHADING,                 // SyncBefore
            D3D12_BARRIER_SYNC_COPY,                        // SyncAfter
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,          // AccessBefore
            D3D12_BARRIER_ACCESS_COPY_SOURCE,               // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,          // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,                    // LayoutAfter
            raytracingOutput,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),  // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        )
    };
    D3D12_BARRIER_GROUP barrierGroupUnorderedAccessToCopySource[] =
    {
        CD3DX12_BARRIER_GROUP(1, barrierUnorderedAccessToCopySource)
    };
    commandList->Barrier(1, barrierGroupUnorderedAccessToCopySource);

    commandList->CopyResource(renderTarget, raytracingOutput);
    commandList->CopyResource(m_raytracingOutputPrevFrame.Get(), raytracingOutput);

    D3D12_RESOURCE_BARRIER postCopyBarriers[1] = {};
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // Required state for postprocessing.
        //D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    //postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
    //    m_raytracingOutput.Get(),
    //    D3D12_RESOURCE_STATE_COPY_SOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    //);
    //postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
    // ambientMap, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
    // renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, postCopyBarriers);

    D3D12_TEXTURE_BARRIER barrierCopySourceToUnorderedAccess[] =
    {
        CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_COPY,                        // SyncBefore
            D3D12_BARRIER_SYNC_ALL_SHADING,                         // SyncAfter
            D3D12_BARRIER_ACCESS_COPY_SOURCE,               // AccessBefore
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                    // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,                    // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,                    // LayoutAfter
            raytracingOutput,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),  // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        )
    };
    D3D12_BARRIER_GROUP barrierGroupCopySourceToUnorderedAccess[] =
    {
        CD3DX12_BARRIER_GROUP(1, barrierCopySourceToUnorderedAccess)
    };
    commandList->Barrier(1, barrierGroupCopySourceToUnorderedAccess);

    // Hdr render texture state was updated directly, so update its state data member.
    m_renderTex[RenderTextures::Hdr]->UpdateState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Game::SetBlurConstants(uint32_t blurMap0UavId, uint32_t blurMap1UavId, uint32_t normalDepthMapSrvId, const float* weights, bool isAoBlur)
{
    m_blurConstants->blurMap0UavID = blurMap0UavId;
    m_blurConstants->blurMap1UavID = blurMap1UavId;
    m_blurConstants->normalDepthMapSrvID = normalDepthMapSrvId;

    m_blurConstants->blurWeights[0] = Vector4(&weights[0]);
    m_blurConstants->blurWeights[1] = Vector4(&weights[4]);
    // The third vector of weights is only required for the ambient occlusion blur.
    if (isAoBlur)
        m_blurConstants->blurWeights[2] = Vector4(&weights[8]);
}

// Release resources that are dependent on the size of the main window.
//void Game::ReleaseWindowSizeDependentResources()
//{
//    //m_shaderTableRadiance.reset();
//    //m_shaderTableAmbientOcclusion.reset();
//    //m_rayGenShaderTable.reset();
//    //m_hitGroupShaderTable.reset();
//    //m_missShaderTable.reset();
//
//    //m_rayGenShaderTable_ao->buffer.Reset();
//    //m_hitGroupShaderTable_ao->buffer.Reset();
//    //m_missShaderTable_ao->buffer.Reset();
//    //
//    //m_rayGenShaderTable->buffer.Reset();
//    //m_hitGroupShaderTable->buffer.Reset();
//    //m_missShaderTable->buffer.Reset();
//    //m_missShaderTable.Reset();
//}

// Release all resources that depend on the device.
//void Game::ReleaseDeviceDependentResources()
//{
//}

/*
// Create SRV for a buffer.
void Game::CreateBufferSRV(
ID3D12Resource* buffer,
uint32_t numElements,
uint32_t elementSize,
uint32_t descriptorIndex)
{
    auto device = m_deviceResources->GetD3DDevice();

    // SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.NumElements = numElements;

    if (elementSize == 0)
    {    // index buffer srv
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // raw buffers cannot be created with r16 typeless format
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        srvDesc.Buffer.StructureByteStride = 0;
    }
    else
    {   // vertex buffer srv
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        srvDesc.Buffer.StructureByteStride = elementSize;
    }

    //buffer->cpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvUavDescHeap->GetCpuHandle(descriptorIndex));
    //UINT descriptorIndex = AllocateDescriptor(&buffer->cpuDescriptorHandle);

    device->CreateShaderResourceView(
    buffer,
    &srvDesc,
    m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(descriptorIndex)
    );
    //device->CreateShaderResourceView(
    buffer->resource.Get(),
    &srvDesc, buffer->cpuDescriptorHandle
    );
    //buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvUavDescHeap->GetGpuHandle(descriptorIndex));
    //return descriptorIndex;
}
*/
