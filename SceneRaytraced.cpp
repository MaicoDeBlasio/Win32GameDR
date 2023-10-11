#include "pch.h"
#include "Game.h"   // Forward declared classes must be defined with its header added to the .cpp file
#include "SceneRaytraced.h"

SceneRaytraced::SceneRaytraced() noexcept
//SceneRaytraced::SceneRaytraced(Game* game) noexcept : Scene(game) {}
{
    m_tlasBuffers = std::make_unique<AccelerationStructureBuffers>();
}

void SceneRaytraced::BuildTLAS(
    ID3D12Device10* device,
    ID3D12GraphicsCommandList7* commandList,
    AccelerationStructureBuffers* TLAS,
    //D3D12_RAYTRACING_INSTANCE_DESC* tlasInstanceDesc,
    uint32_t numDescs,
    bool isUpdate)
    //void Game::BuildTopLevelAS(std::vector<AccelerationStructureBuffers>& bottomLevelAS, bool isUpdate)
    //void Game::BuildTopLevelAS(std::vector<AccelerationStructureBuffers>& bottomLevelAS)
    //void Game::BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count])
    //AccelerationStructureBuffers Game::BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count],
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags, bool fullBuild)
    //AccelerationStructureBuffers Game::BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count],
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
    //AccelerationStructureBuffers Game::BuildTopLevelAS(AccelerationStructureBuffers bottomLevelAS[GeometryType::Count],
    // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
    // *** Important notes about this method ***
    // The original bug was that if this method was called after setting the srv descriptor heap, then ambient and normal-depth textures would be
    // intermittently available, improving at small window sizes but completely absent for fullscreen.
    // Setting the descriptor heap afterwards did seem to solve the bug, but the unstable behaviour became default after updating to Nvidia driver v531.
    // Proper performance was restored By re-adding _ALLOW_UPDATE flag to _AS_STRUCTURE INPUTS, and _PERFORM_UPDATE flag to _AS_STRUCTURE_DESC, as well as
    // reinstating UAV barrriers both before and after TLAS update.
    // With these changes, the application would run for some seconds before crashing. The solution so far seems to be simply removing the _PREFER_FAST_TRACE
    // flag from _AS_STRUCTURE_INPUTS.
    // I will now test this solution against the latest Nvidia driver.

    //auto device = m_deviceResources->GetD3DDevice();
    //auto commandList = m_deviceResources->GetCommandList();
    //auto commandAllocator = m_deviceResources->GetCommandAllocator();

    //ComPtr<ID3D12Resource> scratch;
    //ComPtr<ID3D12Resource> topLevelAS;

    // Get required sizes for an acceleration structure.
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD
        //topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
        | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    topLevelInputs.NumDescs = numDescs;// TLASInstances::Count; // three triangle instances + ground plane
    //topLevelInputs.NumDescs = m_geometryInstanceCount + 1;
    const auto dataSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numDescs;

    //auto& topLevelBuildDesc = m_tlasBuffers.m_topLevelBuildDesc;
    //topLevelBuildDesc = {};
    //D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    //D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;

    //topLevelBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    //topLevelBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    //topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;// buildFlags;
    //topLevelBuildDesc.Inputs.NumDescs = m_geometryInstanceCount + 1; // three triangle instances + ground plane

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    //device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelBuildDesc.Inputs, &topLevelPrebuildInfo);
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    DX::ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

    const auto syncBits = D3D12_BARRIER_SYNC_ALL_SHADING |
        D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE |
        D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE |
        D3D12_BARRIER_SYNC_EMIT_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO;
    const auto accessBits = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS |
        D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE |
        D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
    if (isUpdate)
    {
        // If this a request for an update, then the TLAS was already used in a DispatchRay() call.
        // We need a UAV barrier to make sure the read operation ends before updating the buffer.
        D3D12_BUFFER_BARRIER uavBarrier = CD3DX12_BUFFER_BARRIER(
            syncBits, syncBits, accessBits, accessBits, TLAS->accelerationStructure.Get()
        );
        D3D12_BARRIER_GROUP uavBarrierGroup = CD3DX12_BARRIER_GROUP(1, &uavBarrier);
        commandList->Barrier(1, &uavBarrierGroup);

        //D3D12_RESOURCE_BARRIER uavBarrier = {};
        //D3D12_RESOURCE_BARRIER uavBarrier[2] = {};
        //uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(TLAS->accelerationStructure.Get());
        //uavBarrier[1] = CD3DX12_RESOURCE_BARRIER::UAV(TLAS->scratch.Get());
        //uavBarrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_tlasBuffers->accelerationStructure.Get());
        //uavBarrier[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_tlasBuffers->scratch.Get());
        //uavBarrier[2] = CD3DX12_RESOURCE_BARRIER::UAV(m_tlasBuffers.instanceDesc.Get());
        //uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        //uavBarrier.UAV.pResource = m_tlasBuffers.accelerationStructure.Get();
        //commandList->ResourceBarrier(1, &uavBarrier); // This is required.
        //commandList->ResourceBarrier(2, uavBarrier); // This is required.
        //}
        //
        //// Create instance descs for the bottom-level acceleration structures.
        ////ComPtr<ID3D12Resource> instanceDescsResource;
        //if (isUpdate)
        //{
            // Not referenced.
            //D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[GeometryType::Count] = {};
            //std::vector<D3D12_GPU_VIRTUAL_ADDRESS> bottomLevelASaddresses =
            //    //D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASaddresses[TriangleMeshes::Count] =
            //    //D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASaddresses[GeometryType::Count] =
            //{
            //    m_blasBuffers[TriangleMeshes::Cube].accelerationStructure->GetGPUVirtualAddress(),
            //    m_blasBuffers[TriangleMeshes::Plane].accelerationStructure->GetGPUVirtualAddress()
            //    //bottomLevelAS[TriangleMeshes::Cube].accelerationStructure->GetGPUVirtualAddress(),
            //    //bottomLevelAS[TriangleMeshes::Plane].accelerationStructure->GetGPUVirtualAddress()
            //};
            // Important - do not pass pointer address as "&m_tlasBuffers.instanceDesc" or "m_tlasBuffers.instanceDesc.ReleaseAndGetAddressOf()".
            // Both pointers will be released and a 'resource referenced by gpu operations in-flight on command queue' corruption debug error will occur.

            //BuildTLASInstanceDescs(tlasInstanceDesc);
        memcpy(m_pMappedData, m_tlasInstanceDesc.get(), dataSize);
        //BuildTLASInstanceDescs();
        //BuildBottomLevelASInstanceDescs(bottomLevelASaddresses);
        //BuildBottomLevelASInstanceDescs(bottomLevelASaddresses, m_tlasBuffers.instanceDesc.GetAddressOf()); // must pass a pointer to pointer
        //m_tlasBuffers.instanceDesc->SetName(L"TLAS instance");
        //BuildBottomLevelASInstanceDescs(bottomLevelASaddresses, &instanceDescsResource);
        //BuildBottomLevelASInstanceDescs<D3D12_RAYTRACING_INSTANCE_DESC>(bottomLevelASaddresses, &instanceDescsResource);
    }
    else
    {
        AllocateUAVBufferWithLayout(device, topLevelPrebuildInfo.ScratchDataSizeInBytes, &TLAS->scratch, L"TLASScratchResource");
        /*
        AllocateUAVBuffer(
            device,
            topLevelPrebuildInfo.ScratchDataSizeInBytes,
            &TLAS->scratch,
            //&m_tlasBuffers->scratch,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            L"ScratchResource"
        );
        */
        //AllocateUAVBuffer(device, topLevelPrebuildInfo.ScratchDataSizeInBytes, &scratch,
        // D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

        // Allocate resources for acceleration structures.
        // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
        // Default heap is OK since the application doesn’t need CPU read/write access to them. 
        // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
        // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
        //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
        //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
        //D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        AllocateUAVBufferWithLayout(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &TLAS->accelerationStructure, L"TLAS", true);
        /*
        AllocateUAVBuffer(
            device,
            topLevelPrebuildInfo.ResultDataMaxSizeInBytes,
            &TLAS->accelerationStructure,
            //&m_tlasBuffers->accelerationStructure,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            L"TopLevelAccelerationStructure"
        );
        */
        //AllocateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &topLevelAS,
        // initialResourceState, L"TopLevelAccelerationStructure");

        // Create TLAS shader resource view for direct descriptor heap indexing in shader.
        const auto descHeap = m_game->GetDescriptorHeap(DescriptorHeaps::SrvUav);
        CreateTLASBufferShaderResourceView(device, TLAS->accelerationStructure->GetGPUVirtualAddress(),
            descHeap->GetCpuHandle(SrvUAVs::TLASBufferSrv));
        //m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::TLASSrv));
        //CreateTLASBufferShaderResourceView(device, m_tlasBuffers->accelerationStructure->GetGPUVirtualAddress(),
        //    m_descHeap[DescriptorHeaps::SrvUav]->GetCpuHandle(SrvUAVs::TLASSrv));

        AllocateUploadBufferWithLayout(device, dataSize, &TLAS->instanceDesc, L"InstanceDescBuffer");
        /*
        AllocateUploadBuffer(
            device,
            //reinterpret_cast<void*>(tlasInstanceDesc),
            //reinterpret_cast<void**>(&m_tlasInstanceDesc),
            dataSize,
            //sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * topLevelInputs.NumDescs,
            &TLAS->instanceDesc,
            //&m_tlasBuffers->instanceDesc,
            L"InstanceDesc"
        );
        */
        //AllocateUploadBuffer(device, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * topLevelInputs.NumDescs,
        // &m_tlasBuffers.instanceDesc, L"InstanceDesc");
        //AllocateUploadBuffer(device, &m_instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 4,
        // &m_tlasBuffers.instanceDesc, L"InstanceDesc");

        //BuildTLASInstanceDescs(tlasInstanceDesc);

        // NB: Mapping has been moved to AllocateUploadBuffer() function body.
        // Edit: Mapping has been removed from AllocateUploadBuffer() function body.
        // Map the instance desc buffer. Note that unlike D3D11, the resource does not need to be unmapped for use by the GPU.
        // The resource stays 'permanently' mapped to avoid overhead with mapping/unmapping each frame.
        //CD3DX12_RANGE readRange(0, 0);
        //void* pMappedData = {};
        DX::ThrowIfFailed(TLAS->instanceDesc->Map(0, nullptr, &m_pMappedData));
        memcpy(m_pMappedData, m_tlasInstanceDesc.get(), dataSize);
        //ThrowIfFailed(TLAS->instanceDesc->Map(0, nullptr, reinterpret_cast<void**>(tlasInstanceDesc)));
        //ThrowIfFailed(TLAS->instanceDesc->Map(0, nullptr, reinterpret_cast<void**>(m_tlasInstanceDesc.get())));

        //BuildBLASInstanceDescs();

        //D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs{};
        //std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(4);
        //instanceDescs.resize(4); // 3 triangle instances + 1 ground plane
        //ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 4u);

        /*
        // Bottom-level AS with instanced triangle polygon.
        for (UINT i = 0; i < m_geometryInstanceCount; i++)
        {
            auto& instanceDesc = m_instanceDescs[i];
            instanceDesc = {};
            // Instance flags, including backface culling, winding, etc - TODO: should
            // be accessible from outside.
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
            // Instance ID visible in the shader in InstanceID()
            instanceDesc.InstanceID = i;
            instanceDesc.InstanceMask = 1;//0xFF
            // Index of the hit group invoked upon intersection
            instanceDesc.InstanceContributionToHitGroupIndex = i * RayType::Count;
            //instanceDesc.InstanceContributionToHitGroupIndex = 0;
            instanceDesc.AccelerationStructure = m_blasBuffers[TriangleMeshes::Cube].accelerationStructure->GetGPUVirtualAddress();
            //instanceDesc.AccelerationStructure = bottomLevelASaddresses[GeometryType::Triangle];
            memcpy(instanceDesc.Transform, &m_geometryTransforms[i], sizeof(instanceDesc.Transform)); // this works!
        }

        // Bottom-level AS with a single plane.
        {
            auto& instanceDesc = m_instanceDescs[3];
            instanceDesc = {};
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
            // Instance ID visible in the shader in InstanceID()
            instanceDesc.InstanceID = 1;
            instanceDesc.InstanceMask = 1;
            // Set hit group offset to beyond the shader records for the triangle.
            instanceDesc.InstanceContributionToHitGroupIndex = 3 * RayType::Count; // plane is associated with the next hit group!
            //instanceDesc.InstanceContributionToHitGroupIndex = 1; // plane is associated with the next hit group!
            instanceDesc.AccelerationStructure = m_blasBuffers[TriangleMeshes::Plane].accelerationStructure->GetGPUVirtualAddress();
            //instanceDesc.AccelerationStructure = bottomLevelASaddresses[GeometryType::Triangle];

            // Calculate transformation matrix.
            auto transform = Matrix::Identity;
            XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(m_instanceDescs[3].Transform), transform);
        }
        */
    }

    // Top-level AS desc
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    topLevelBuildDesc.Inputs = topLevelInputs;
    topLevelBuildDesc.DestAccelerationStructureData = TLAS->accelerationStructure->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = TLAS->scratch->GetGPUVirtualAddress();
    //topLevelBuildDesc.DestAccelerationStructureData = m_tlasBuffers->accelerationStructure->GetGPUVirtualAddress();
    //topLevelBuildDesc.ScratchAccelerationStructureData = m_tlasBuffers->scratch->GetGPUVirtualAddress();
    //topLevelBuildDesc.DestAccelerationStructureData = topLevelAS->GetGPUVirtualAddress();
    //topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
    topLevelBuildDesc.Inputs.InstanceDescs = TLAS->instanceDesc->GetGPUVirtualAddress();
    //topLevelBuildDesc.Inputs.InstanceDescs = m_tlasBuffers->instanceDesc->GetGPUVirtualAddress();
    //topLevelInputs.InstanceDescs = m_tlasBuffers.instanceDesc->GetGPUVirtualAddress();
    //topLevelInputs.InstanceDescs = instanceDescsResource->GetGPUVirtualAddress();

    // If this is an update operation, set the source buffer and the perform_update flag
    // NB: Was advised by Adam Miles to avoid TLAS "refitting" and rebuilding instead.
    // This can be achieved by not specifying the _PERFORM_UPDATE flag.
    // Edit: Reimplementing this flag helped solve the rtao raytracing instability bug!

    if (isUpdate)
    {
        topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        topLevelBuildDesc.SourceAccelerationStructureData = TLAS->accelerationStructure->GetGPUVirtualAddress();
        //topLevelBuildDesc.SourceAccelerationStructureData = m_tlasBuffers->accelerationStructure->GetGPUVirtualAddress();

        // Reset the command list for the acceleration structure construction.
        //commandList->Reset(m_deviceResources->GetCommandAllocator(), nullptr);
    }

    // Build acceleration structure.
    commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    /*
    if (isUpdate)
    {
        // We need to insert a UAV barrier before using the acceleration structures in a raytracing operation.
        D3D12_RESOURCE_BARRIER uavBarrier[2] = {};
        uavBarrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(TLAS->accelerationStructure.Get());
        uavBarrier[1] = CD3DX12_RESOURCE_BARRIER::UAV(TLAS->scratch.Get());
        //uavBarrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_tlasBuffers->accelerationStructure.Get());
        //uavBarrier[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_tlasBuffers->scratch.Get());
        commandList->ResourceBarrier(2, uavBarrier);
        //D3D12_RESOURCE_BARRIER uavBarrier = {};
        //uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        //uavBarrier.UAV.pResource = m_tlasBuffers.accelerationStructure.Get();
        //commandList->ResourceBarrier(1, &uavBarrier);

        //m_deviceResources->ExecuteCommandList();
        //m_deviceResources->WaitForGpu();
        //m_tlasBuffers.accelerationStructure = topLevelAS;
        //m_tlasBuffers.scratch = scratch;
        //m_tlasBuffers.ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    }
    */
    //m_tlasBuffers.instanceDesc = instanceDescsResource;

    //AccelerationStructureBuffers topLevelASBuffers;
    //topLevelASBuffers.accelerationStructure = topLevelAS;
    //topLevelASBuffers.instanceDesc = instanceDescsResource;
    //topLevelASBuffers.scratch = scratch;
    //topLevelASBuffers.ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;
    //return topLevelASBuffers;
}
