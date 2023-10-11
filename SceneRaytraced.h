#pragma once
#include "Scene.h"

class SceneRaytraced : public Scene
{
public:

    SceneRaytraced() noexcept;
    //SceneRaytraced(Game* game) noexcept;

    SceneRaytraced(SceneRaytraced const&) = delete;
    SceneRaytraced& operator= (SceneRaytraced const&) = delete;

    SceneRaytraced(SceneRaytraced&&) = default;
    SceneRaytraced& operator= (SceneRaytraced&&) = default;

    ~SceneRaytraced() = default;

protected:

    virtual void CreateInstanceBuffer(ID3D12Device* device, ID3D12CommandQueue* commandQueue) = 0;
    virtual void BuildBLASGeometryDescs() = 0;
    virtual void BuildStaticBLAS(ID3D12Device10* device, ID3D12GraphicsCommandList7* commandList) = 0;
    virtual void BuildDynamicBLAS(ID3D12Device10* device, ID3D12GraphicsCommandList7* commandList, bool isUpdate) = 0;
    virtual void BuildTLASInstanceDescs() = 0;

    // This method is implemented by this class.
    void BuildTLAS(
        ID3D12Device10* device,
        ID3D12GraphicsCommandList7* commandList,
        AccelerationStructureBuffers* TLAS,
        //D3D12_RAYTRACING_INSTANCE_DESC* tlasInstanceDesc,
        uint32_t numDescs,
        bool isUpdate);

    void* m_pMappedData;

    std::unique_ptr<AccelerationStructureBuffers>   m_tlasBuffers;
    std::unique_ptr<AccelerationStructureBuffers[]> m_blasBuffers[BLASType::Count];

    std::unique_ptr<D3D12_RAYTRACING_GEOMETRY_DESC[]> m_geometryDesc;

    // We map the TLAS instance buffer to this struct so the buffer can be updated per frame.
    std::unique_ptr<D3D12_RAYTRACING_INSTANCE_DESC[]> m_tlasInstanceDesc;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceStructBuffer;
};

