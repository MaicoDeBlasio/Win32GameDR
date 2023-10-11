//
// Game.h
//

#pragma once

#include "DirectXRaytracingHelper.h"
#include "RaytracingHlslCompat.h"
#include "CollisionStructs.h"
#include "AnimationStructs.h"
#include "NameSpacedEnums.h"
#include "DeviceResources.h"
#include "OcclusionManager.h"
//#include "RaytracedShadows.h"
#include "RenderTexture.h"
#include "SDKMESHModel.h"
//#include "RaytracedAO.h"
#include "StepTimer.h"
#include "FBXModel.h"
#include "Camera.h"

#include "SceneMain.h"

// A basic game implementation that creates a D3D12 device and provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

    // Scene pointer
    //const auto GetScenePointer() const { return this; };

private:

    //void CalculateFrameStats(DX::StepTimer const& timer);
    //void Update();
    //void Update(DX::StepTimer const& timer);
    //void Render();

    //void Clear();
    void ApplyToneMapping(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE hdrDescriptor);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void UpdateForSizeChange(int width, int height);

    //bool m_isRaster;    // Rasterisation pipeline flag

    // Viewport dimensions.
    int   m_width;
    int   m_height;
    float m_aspectRatio;

    Scene* m_scene;     // Current scene pointer.

    // Scene instances.
    std::unique_ptr<SceneMain> m_mainScene;

    // Static sampler array
    const D3D12_STATIC_SAMPLER_DESC m_staticSamplers[5] = {
        CommonStates::StaticAnisotropicClamp(0),
        CommonStates::StaticLinearClamp(1),
        CommonStates::StaticPointClamp(2),
        CommonStates::StaticAnisotropicWrap(3),
        CommonStates::StaticLinearWrap(4),
    };

    // Device resources
    std::unique_ptr<DX::DeviceResources> m_deviceResources;

    // Rendering loop timer
    std::unique_ptr<DX::StepTimer> m_timer;
    //DX::StepTimer m_timer;

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    std::unique_ptr<GraphicsMemory> m_graphicsMemory;

    // Common rendering states
    //std::unique_ptr<DirectX::CommonStates> m_commonStates;

    // Descriptor heaps
    std::unique_ptr<DescriptorHeap> m_descHeap[DescriptorHeaps::Count];
    //std::unique_ptr<DirectX::DescriptorHeap> m_srvUavHeap;
    //std::unique_ptr<DirectX::DescriptorHeap> m_rtvHeap;

    // Effect texture factories
    std::unique_ptr<EffectTextureFactory> m_texFactory;

    // Render targets
    std::unique_ptr<DX::RenderTexture> m_renderTex[RenderTextures::Count];
    //std::unique_ptr<DX::RenderTexture> m_hdrTex;

    // Postprocessing resources
    std::unique_ptr<BasicPostProcess> m_basicPostProc[PostProcessType::Count];
    std::unique_ptr<DualPostProcess>  m_bloomCombine;

    // Tone mapping resources
    std::unique_ptr<ToneMapPostProcess> m_toneMap[TonemapType::Count];
    //std::unique_ptr<DirectX::ToneMapPostProcess> m_toneMap;
    //std::unique_ptr<DirectX::ToneMapPostProcess> m_toneMapHDR10;
    //std::unique_ptr<DirectX::ToneMapPostProcess> m_toneMapLinear;

    // Mouse & keyboard input
    std::unique_ptr<Keyboard> m_keyboard;
    std::unique_ptr<Mouse> m_mouse;

    std::unique_ptr<Keyboard::KeyboardStateTracker> m_keyTracker;
    std::unique_ptr<Mouse::ButtonStateTracker> m_mouseTracker;

    // Root signatures
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig[RootSignatures::Count];
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_computeBlurRootSignature;
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState[PSOs::Count];
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_cubesPipelineState;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_planePipelineState;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_staticModelPipelineState;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_cubeMapPipelineState;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_blurHorzPipelineState;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_blurVertPipelineState;

    // Geometry resources
    //std::unique_ptr<DirectX::SimpleMath::Matrix[]> m_cubeTransforms;
    //std::unique_ptr<XMFLOAT3X4[]> m_cubeTransforms;
    //std::unique_ptr<ObjectConstants[]> m_cubeConstants;
    //std::unique_ptr<ObjectConstants> m_planeConstants;
    //ObjectConstants m_planeConstants;

    std::unique_ptr<ProcGeometryBuffersAndViews[]> m_procGeometry;
    //std::unique_ptr<GeometryBuffers> m_cubeBuffers;
    //std::unique_ptr<GeometryBuffers> m_planeBuffers;

    // Resource textures and buffers
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_structBuffer[StructBuffers::Count];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_raytracingOutput;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_raytracingOutputPrevFrame;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_structuredBuffer; // new
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_structuredBuffer_ao; // new

    //Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    //
    //D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    //D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;
    //D3D12_VERTEX_BUFFER_VIEW m_materialInstanceBufferView;
    //D3D12_INDEX_BUFFER_VIEW  m_indexBufferView;

    //static const UINT m_geometryInstanceCount = 3;

    // Plane
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_planeVertexBuffer;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_planeIndexBuffer;
    //
    //D3D12_VERTEX_BUFFER_VIEW m_planeVertexBufferView;
    //D3D12_INDEX_BUFFER_VIEW  m_planeIndexBufferView;

    // Static models
    std::unique_ptr<SDKMESHModel> m_SDKMESHModel[SDKMESHModels::Count];
    //std::unique_ptr<SDKMESHModel> m_suzanne;
    //std::unique_ptr<SDKMESHModel> m_racetrack;
    //std::unique_ptr<SDKMESHModel> m_palmtree;
    //std::unique_ptr<SDKMESHModel> m_miniracecar;
    //std::unique_ptr<StaticModel> m_palmtree_trunk;  // For DXR test.
    //std::unique_ptr<StaticModel> m_racetrack_grass;
    //std::unique_ptr<StaticModel> m_racetrack_road;

    // FBX models
    std::unique_ptr<FBXModel> m_FBXModel[FBXModels::Count];
    //std::unique_ptr<FBXModel> m_dove;

    // Geometric primitives
    std::unique_ptr<DirectX::GeometricPrimitive> m_geospherePrimitive;  // Viewed from inside.

    // Raytraced occlusion manager instances.
    std::unique_ptr<OcclusionManager> m_aoManager;
    std::unique_ptr<OcclusionManager> m_shadowManager;
    //std::unique_ptr<RaytracedAO>      m_raytracedAO;
    //std::unique_ptr<RaytracedShadows> m_raytracedShadows;

    std::unique_ptr<BlurConstants> m_blurConstants;
    //AoBlurWeights m_aoBlurWeights;

    // Collision structures.
    //DirectX::BoundingSphere m_cameraSphere; // test for collision with first person camera

    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_commandBuffer;
    std::unique_ptr<ConstantBuffer<FrameConstants>> m_constantBufferIndirect;

private:

    // DirectX Raytracing (DXR) resources.

    //Microsoft::WRL::ComPtr<ID3D12Device5> m_dxrDevice;
    //Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;
    Microsoft::WRL::ComPtr<ID3D12StateObject> m_stateObject[StateObjects::Count];
    //Microsoft::WRL::ComPtr<ID3D12StateObject> m_dxrStateObject;
    //Microsoft::WRL::ComPtr<ID3D12StateObject> m_dxrStateObject_ao;

    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature_ao;
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature_ao;

    // Raytracing scene
    //RayGenConstantBuffer m_rayGenCB;

    // Acceleration structures
    //std::unique_ptr<AccelerationStructureBuffers>   m_tlasBuffers;
    //AccelerationStructureBuffers m_tlasBuffers;
    //std::unique_ptr<AccelerationStructureBuffers[]> m_blasBuffers[BLASType::Count];
    //std::unique_ptr<AccelerationStructureBuffers[]> m_blasBuffers;
    //std::vector<AccelerationStructureBuffers> m_blasBuffers;
    //AccelerationStructureBuffers m_blasBuffers[TriangleMeshes::Count];

    //std::unique_ptr<D3D12_RAYTRACING_GEOMETRY_DESC[]> m_geometryDesc;

    // We map the TLAS instance buffer to this struct so the buffer can be updated per frame.
    // Using a shared_ptr instead of unique_ptr fixes 'Invalid address specified to RtlValidateHeap' error
    // at application shutdown.
    //std::shared_ptr<D3D12_RAYTRACING_INSTANCE_DESC[]> m_tlasInstanceDesc;
    //void* m_pMappedData;
    //std::unique_ptr<D3D12_RAYTRACING_INSTANCE_DESC[]> m_instanceDescs;
    //D3D12_RAYTRACING_INSTANCE_DESC* m_instanceDescs;

    //Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomLevelAS[TriangleMeshes::Count];
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomLevelAS[GeometryType::Count];
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_topLevelAS;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_accelerationStructure;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

    // Raytracing output
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_raytracingOutput_ao;
    //D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;
    //UINT m_raytracingOutputResourceUAVDescriptorHeapIndex;

    // Shader tables.
    static const wchar_t* c_raygenShaderName;
    static const wchar_t* c_closestHitShaderNames[2];
    //static const wchar_t* c_closestHitShaderNames[MeshType::Count];
    //static const wchar_t* c_closestHitShaderName;
    static const wchar_t* c_anyHitShaderName; // Only one any hit shader required for transparent geometry.
    //static const wchar_t* c_anyHitShaderNames[RayType::Count];
    static const wchar_t* c_missShaderNames[RayType::Count];
    static const wchar_t* c_hitGroupNames[MeshType::Count][RayType::Count];
    //static const wchar_t* c_hitGroupNames[MeshType::Count];
    //static const wchar_t* c_hitGroupNames[MeshInstances::Count][RayType::Count]; // (3 cube instances + plane) * ray types
    //static const wchar_t* c_hitGroupNames[m_geometryInstanceCount+1][RayType::Count];
    //static const wchar_t* c_hitGroupNames[2];
    //static const wchar_t* c_hitGroupName;

    // Ambient occlusion pass shaders.
    //static const wchar_t* c_hitGroupNames_ao[MeshInstances::Count][RayType::Count];
    //static const wchar_t* c_hitGroupNames_ao[m_geometryInstanceCount + 1][RayType::Count];
    //static const wchar_t* c_missShaderNames_ao[RayType::Count];

    std::unique_ptr<ShaderBindingTableGroup[]> m_shaderBindingTableGroup;
    //std::unique_ptr<ShaderTableCollection> m_shaderTableRadiance;
    //std::unique_ptr<ShaderTableCollection> m_shaderTableAmbientOcclusion;

    //std::unique_ptr<ShaderTableBuffer> m_missShaderTable;
    //std::unique_ptr<ShaderTableBuffer> m_hitGroupShaderTable;
    //std::unique_ptr<ShaderTableBuffer> m_rayGenShaderTable;

    //Microsoft::WRL::ComPtr<ID3D12Resource> m_missShaderTable;
    //uint32_t m_missShaderTableStrideInBytes;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_hitGroupShaderTable;
    //uint32_t m_hitGroupShaderTableStrideInBytes;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_rayGenShaderTable;

    // Ambient occlusion shader table resources
    //std::unique_ptr<ShaderTableBuffer> m_missShaderTable_ao;
    //std::unique_ptr<ShaderTableBuffer> m_hitGroupShaderTable_ao;
    //std::unique_ptr<ShaderTableBuffer> m_rayGenShaderTable_ao;

    //Microsoft::WRL::ComPtr<ID3D12Resource> m_missShaderTable_ao;
    //uint32_t m_missShaderTableStrideInBytes_ao;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_hitGroupShaderTable_ao;
    //uint32_t m_hitGroupShaderTableStrideInBytes_ao;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_rayGenShaderTable_ao;

    //void RecreateD3D();
    //void DoRaytracing_ao(ID3D12GraphicsCommandList4* commandList);
    //void DoRaytracing_ao();
    //void DoRaytracing();
    //void ReleaseDeviceDependentResources();
    //void ReleaseWindowSizeDependentResources();

    //void CreateRootSignatures(ID3D12Device* device);
    //void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
    //void CreateRaytracingPipelineStateObject(ID3D12Device5* device, const unsigned char* shader, );

    // This function calls:
    //  CreateRaytracingPipelineStateObject()
    //  BuildAccelerationStructures()
    //  BuildShaderTables()

    void CreateGeometryAndMaterialResources(ID3D12Device* device);
    //void BuildGeometryAndMaterials(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
    void CreateRaytracingResources(ID3D12Device5* device);
    
    // If the purpose of a function is to fill an external pointer P, then P** must be passed in as the parameter.
    // With payload access qualifiers (PAQs), the MaxPayloadSizeInBytes property of D3D12_RAYTRACING_SHADER_CONFIG is no longer needed.
    // The field is ignored by drivers if PAQs are enabled (the default) in SM 6.7 or higher.
    void CreateRaytracingPipelineStateObject(
        ID3D12Device5* device,
        const unsigned char* shaderBytes, size_t byteLength, uint32_t payloadSize,
        ID3D12StateObject** stateObject);
    //void CreateRaytracingPipelineStateObject();

    //void BuildAccelerationStructures(
    //    ID3D12Device5* device,
    //    ID3D12GraphicsCommandList4* commandList,
    //    ID3D12CommandAllocator* commandAlloc);

    void BuildShaderTables(
        ID3D12Device5* device,
        Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject,
        ShaderBindingTableGroup* sbtGroup);

    void CreateRaytracingOutputResources(ID3D12Device10* device);
    //void CreateRaytracingOutputResources(ID3D12Device* device);

    //void CopyRaytracingOutputToBackbuffer();
    //void CreateBufferSRV(ID3D12Resource* buffer, uint32_t numElements, uint32_t elementSize, uint32_t descriptorIndex);
    //void CreateBufferSRV(ID3D12Resource* buffer, UINT numElements, UINT elementSize);

    //void CreateByteAddressShaderResourceView(
    //    ID3D12Device* device,
    //    ID3D12Resource* buffer,
    //    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor);

    //void BuildBLASGeometryDescs();
    //void BuildBLASGeometryDescs(D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs);
    //void BuildGeometryDescsForBottomLevelAS(std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs);
    //void BuildGeometryDescsForBottomLevelAS(std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>, 2>& geometryDescs);
    //void BuildGeometryDescsForBottomLevelAS(std::array<std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>,
    // GeometryType::Count>& geometryDescs);

    //void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, bool isFirstBuild);
    //void BuildBottomLevelAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList,
    // const D3D12_RAYTRACING_GEOMETRY_DESC* geometryDescs);
    //AccelerationStructureBuffers BuildBottomLevelAS(const D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
    //AccelerationStructureBuffers BuildBottomLevelAS(const D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
    //AccelerationStructureBuffers BuildBottomLevelAS(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDesc,
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags =
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    
    //template <class InstanceDescType, class BLASPtrType>
    //void BuildTLASInstanceDescs(D3D12_RAYTRACING_INSTANCE_DESC* tlasInstanceDesc);
    //void BuildTLASInstanceDescs();
    //void BuildBLASInstanceDescs();
    //void BuildBottomLevelASInstanceDescs(BLASPtrType* bottomLevelASaddresses,
    // Microsoft::WRL::ComPtr<ID3D12Resource>* instanceDescsResource);
    //void BuildBottomLevelASInstanceDescs(std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& bottomLevelASaddresses);
    //void BuildBottomLevelASInstanceDescs(std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& bottomLevelASaddresses,
    // ID3D12Resource** ppInstanceDescsResource, bool isUpdate); // we are filling the resource
    //void BuildBottomLevelASInstanceDescs(std::vector<D3D12_GPU_VIRTUAL_ADDRESS>& bottomLevelASaddresses,
    // Microsoft::WRL::ComPtr<ID3D12Resource>* instanceDescsResource);
    //void BuildBottomLevelASInstanceDescs(D3D12_GPU_VIRTUAL_ADDRESS* bottomLevelASaddresses,
    // Microsoft::WRL::ComPtr<ID3D12Resource>* instanceDescsResource);

    //void BuildTopLevelAS(bool isUpdate);
    //void BuildTopLevelAS(std::vector<AccelerationStructureBuffers>& bottomLevelAS, bool isUpdate);
    //void BuildTopLevelAS(std::vector<AccelerationStructureBuffers>& bottomLevelAS);
    //void BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count]);
    //AccelerationStructureBuffers BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count],
    //D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags =
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    //AccelerationStructureBuffers BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count],
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags =
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    //AccelerationStructureBuffers BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[GeometryType::Count],
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags =
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);
    //void UpdateTopLevelAS(std::vector<AccelerationStructureBuffers>& bottomLevelAS); // added this
    //void UpdateTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count]); // added this

public:

    const auto GetBackbufferWidth() const noexcept { return m_width; }
    const auto GetBackbufferHeight() const noexcept { return m_height; }

    void Clear(ID3D12GraphicsCommandList* commandList);

    //void BuildStaticBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList);
    //void BuildDynamicBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, bool isUpdate);
    //void BuildTLAS(
    //    ID3D12Device5* device,
    //    ID3D12GraphicsCommandList4* commandList,
    //    AccelerationStructureBuffers* TLAS,
    //    D3D12_RAYTRACING_INSTANCE_DESC* tlasInstanceDesc,
    //    uint32_t numDescs,
    //    bool isUpdate);
    //void BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, AccelerationStructureBuffers* TLAS, bool isUpdate);
    //void BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, bool isUpdate);

    void DoRaytracing(
        ID3D12GraphicsCommandList4* commandList,
        ID3D12StateObject* stateObject,
        const ShaderBindingTableGroup* sbtGroup,
        int width, int height);

    void SetBlurConstants(uint32_t blurMap0UavId, uint32_t blurMap1UavId, uint32_t normalDepthMapSrvId, const float* weights, bool isAoBlur);

    //
    // Game collision and physics.
    //

    void TestGroundCollision(SDKMESHModel* groundModel, const BoundingSphere& sphere, CollisionTriangle& triangle);
    void TestGroundCollision(SDKMESHModel* groundModel, const BoundingBox& box, CollisionTriangle& triangle);

    // Public getters.
    const auto GetDeviceResources() const noexcept { return m_deviceResources.get(); }
    const auto GetTimer() const noexcept { return m_timer.get(); }

    const auto GetMouse() const noexcept { return m_mouse.get(); }
    const auto GetMouseTracker() const noexcept { return m_mouseTracker.get(); }
    const auto GetKeyboard() const noexcept { return m_keyboard.get(); }
    const auto GetKeyTracker() const noexcept { return m_keyTracker.get(); }

    //const auto GetCubeTransforms() const noexcept { return m_cubeTransforms.get(); };

    const auto GetSdkMeshModel(uint32_t i) const noexcept { return m_SDKMESHModel[i].get(); }
    const auto GetFbxModel(uint32_t i) const noexcept { return m_FBXModel[i].get(); }

    const auto GetConstBufferIndirect() const noexcept { return m_constantBufferIndirect.get(); }
    const auto GetCommandBuffer() const noexcept { return m_commandBuffer.Get(); }
    const auto GetCommandSignature() const noexcept { return m_commandSignature.Get(); }
    //const auto GetIndirectCommandSize() const noexcept { return sizeof(IndirectCommand); };

    const auto GetDescriptorHeap(uint32_t i) const noexcept { return m_descHeap[i].get(); }
    const auto GetRootSignature(uint32_t i) const noexcept { return m_rootSig[i].Get(); }
    const auto GetPipelineState(uint32_t i) const noexcept { return m_pipelineState[i].Get(); }

    const auto GetGraphicsMemory() const noexcept { return m_graphicsMemory.get(); }

    const auto GetStateObject(uint32_t i) const noexcept { return m_stateObject[i].Get(); }
    const auto GetShaderTableGroup() const noexcept { return m_shaderBindingTableGroup.get(); }

    const auto GetAOManager() const noexcept      { return m_aoManager.get(); }
    const auto GetShadowManager() const noexcept { return  m_shadowManager.get(); }

    const auto GetBlurConstants() const noexcept { return m_blurConstants.get(); }

    const auto GetProcGeometry() const noexcept { return m_procGeometry.get(); }

    const auto GetRenderTextures() const noexcept { return m_renderTex->get(); }
    const auto GetRaytracingOutput() const noexcept { return m_raytracingOutput.Get(); }

    void CopyRaytracingOutputToBackbuffer(ID3D12GraphicsCommandList7* commandList);

    //const auto GetTLASInstanceDesc() const noexcept { return m_tlasInstanceDesc.get(); }; // temp

    //
    // Game postprocessing
    //

    void PrepareHDRTextureForPostProcessing()
    {
        const auto commandList = m_deviceResources->GetCommandList();
        m_renderTex[RenderTextures::Hdr]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    
    void DoPostprocessing(ID3D12GraphicsCommandList7* commandList);
};
