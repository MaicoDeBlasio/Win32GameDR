//
// Game_DXR_Resources.cpp
//

// Create all DXR resources for the game:
//  CreateRaytracingOutputResources
//	CreateRootSignatures
//	CreateRaytracingPipelineStateObjects
//	BuildAccelerationStructures
//	BuildShaderTables
//  BuildBLASGeometryDescs
//  BuildBLASInstanceDescs
//  BuildBLAS
//  BuildTLAS

#include "pch.h"
#include "Game.h"

//using namespace DirectX;
using namespace DX;
// SimpleMath namespace is declared in RaytracingHlslCompat.h
//using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

// Shader entry points.
const wchar_t* Game::c_raygenShaderName = L"RaygenShader";

//const wchar_t* Game::c_closestHitShaderName = L"MyClosestHitShader";
const wchar_t* Game::c_closestHitShaderNames[] =
{
    L"ClosestHitCubeShader",
    L"ClosestHitShader",
    //L"ClosestHitPlaneShader",
    //L"ClosestHitSuzanneShader",
    //L"ClosestHitSdkMeshShader",
    //L"ClosestHitFbxShader",
};

const wchar_t* Game::c_anyHitShaderName = L"AnyHitShader";
//const wchar_t* Game::c_anyHitShaderNames[] =
//{
//    L"AnyHitShader_CameraRay",
//    L"AnyHitShader_Occlusion",
//    //L"AnyHitPlaneShader",
//    //L"AnyHitSuzanneShader",
//    //L"AnyHitSdkMeshShader",
//    //L"AnyHitFbxShader",
//};

const wchar_t* Game::c_missShaderNames[] =
{
    L"MissShader_CameraRay",
    L"MissShader_Occlusion",
};

//const wchar_t* Game::c_missShaderNames_ao[] =
//{
//    L"MyMissShader",    L"MyMissShader_AORay",
//};

// Hit groups.
//const wchar_t* Game::c_hitGroupNames[MeshType::Count] =
//const wchar_t* Game::c_hitGroupName = L"MyHitGroup";
const wchar_t* Game::c_hitGroupNames[][RayType::Count] =
{
    { L"HitGroupCube_Radiance",       L"HitGroupCube_Occlusion" },
    { L"HitGroupOpaque_Radiance",     L"HitGroupOpaque_Occlusion" },
    { L"HitGroupTransparent_Radiance",L"HitGroupTransparent_Occlusion" },
    //{ L"HitGroupSdkMesh", L"HitGroupSdkMesh_Occlusion" },
    //{ L"HitGroupFbx",  L"HitGroupFbx_Occlusion" },

    //{ L"HitGroupRedCube",   L"HitGroupRedCube_Occlusion" },
    //{ L"HitGroupGreenCube", L"HitGroupGreenCube_Occlusion" },
    //{ L"HitGroupBlueCube",  L"HitGroupBlueCube_Occlusion" },
    ////{ L"HitGroupPlane",     L"HitGroupPlane_Occlusion" },
    //{ L"HitGroupRacetrackGrass",L"HitGroupRacetrackGrass_Occlusion" },
    //{ L"HitGroupRacetrackRoad",L"HitGroupRacetrackRoad_Occlusion" },
    //{ L"HitGroupSuzanne",   L"HitGroupSuzanne_Occlusion" },
    //{ L"HitGroupDove",       L"HitGroupDove_Occlusion" },
};

//const wchar_t* Game::c_hitGroupNames_ao[][RayType::Count] =
//{
//    { L"MyHitGroupRedCube",     L"MyHitGroupRedCube_AORay" },
//    { L"MyHitGroupGreenCube",   L"MyHitGroupGreenCube_AORay" },
//    { L"MyHitGroupBlueCube",    L"MyHitGroupBlueCube_AORay" },
//    { L"MyHitGroupPlane",       L"MyHitGroupPlane_AORay" },
//};

/*
void Game::CreateRootSignatures(ID3D12Device* device)
{
    //auto device = m_deviceResources->GetD3DDevice();

    // Global Root Signatures
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        // The root paramater slot for the output uav cannot be mapped to a uav root descriptor, and must be a descriptor table.
        // Using a uav root descriptor results in the following error:
        // "A Shader is declaring a typed UAV using a register mapped to a root descriptor UAV(ShaderRegister = 0, RegisterSpace = 0).
        //  SRV or UAV root descriptors can only be Raw or Structured buffers."
        // Edit: Adding global & local root signature creation for ambient occlusion pass.

        //const D3D12_STATIC_SAMPLER_DESC staticSamplers[] = {
        //    CommonStates::StaticAnisotropicClamp(0),
        //    CommonStates::StaticLinearClamp(1),
        //    CommonStates::StaticLinearWrap(2),
        //    CommonStates::StaticPointClamp(3),
        //};

        //CD3DX12_DESCRIPTOR_RANGE ranges[2]{};
        //CD3DX12_DESCRIPTOR_RANGE UAVDescriptor{};

        //ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // output texture (u0)
        //ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1);  // 4 static index & vertex buffer textures + plane texture (t1-t5)
        //ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);  // 4 static index & vertex buffer textures (t1-t4)
        //UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

        //CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSigParams::Count] = {};
        ////rootParameters[GlobalRootSigParams::OutputViewSlot].InitAsDescriptorTable(1, &UAVDescriptor);
        ////rootParameters[GlobalRootSigParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);   // u0
        ////rootParameters[GlobalRootSigParams::VertexBufferSlot].InitAsDescriptorTable(1, &ranges[1]); // t1-t5
        //rootParameters[GlobalRootSigParams::AccelerationStructure].InitAsShaderResourceView(0); // t0
        //rootParameters[GlobalRootSigParams::CameraConstants].InitAsConstantBufferView(0);        // b0
        //
        //D3D12_ROOT_SIGNATURE_FLAGS rsFlags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
        //auto globalRootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(
        //    ARRAYSIZE(rootParameters), rootParameters,
        //    ARRAYSIZE(m_staticSamplers), m_staticSamplers,
        //    rsFlags
        //);

        // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
        //ThrowIfFailed(CreateRootSignature(device, &globalRootSignatureDesc, &m_rootSig[RootSignatures::DxrGlobal]));
        //ThrowIfFailed(CreateRootSignature(device, &globalRootSignatureDesc, m_raytracingGlobalRootSignature.GetAddressOf()));
        //SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);

        // Create global root signature for ambient occlusion pass.
        //ThrowIfFailed(CreateRootSignature(device, &globalRootSignatureDesc, m_raytracingGlobalRootSignature_ao.GetAddressOf()));
    }

    // Local Root Signatures for color and ambient occlusion ray tracing passes
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    // NB: Local arguments need to be passed as root constants.
    // Edit: We are now indexing into a structured buffer to obtain local arguments, so default local rootsig is reinstated.

    // Color ray tracing pass
    {
        //CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSigParams::Count]{};
        // // b0 was bound in global root sig.
        //rootParameters[LocalRootSigParams::MaterialConstantSlot].InitAsConstants(SizeOfInUint32(ObjectConstants), 1, 0);
        //rootParameters[LocalRootSigParams::ViewportConstantSlot].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);

        // Just create a default local rootsig for the AO pass since we are not binding any local resources.
        auto localRootSignatureDesc = CD3DX12_ROOT_SIGNATURE_DESC(D3D12_DEFAULT);
        //CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
        ThrowIfFailed(CreateRootSignature(device, &localRootSignatureDesc, &m_rootSig[RootSignatures::DxrLocal]));
        //SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
    }

    // Ambient occlusion ray tracing pass
    // Edit: We are replacing the default local rootsig because we wish to bind unique local arguments.
    // Edit: We are now indexing into a structured buffer to obtain local arguments, so default local rootsig is reinstated.
    // Local rootsig for DXR color pass should also suffice if ao pass doesn't require binding local arguments.

    //{
    //    //CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSigParams::Count]{};
    // // b0 was bound in global root sig.
    //    //rootParameters[LocalRootSigParams::MaterialConstantSlot].InitAsConstants(1, 1, 0);
    //    //CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
    //
    //    // Just create a default local rootsig for the AO pass since we are not binding any local resources.
    //    CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(D3D12_DEFAULT);
    //    localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    //
    //    // Combines the D3D12SerializeRootSignature and CreateRootSignature calls which are typically used together.
    //    ThrowIfFailed(CreateRootSignature(device, &localRootSignatureDesc, m_raytracingLocalRootSignature_ao.GetAddressOf()));
    //}
}
*/

// Local root signature and shader association
// This is a root signature that enables a shader to have unique arguments that come from shader tables.
//void Game::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
//{
//    // Hit group and miss shaders in this sample are not using a local root signature and thus one is not associated with them.
//    // Local root signature to be used in a ray gen shader.
//    // Edit: Hit groups for cubes & plane require external data (material root constants) so associate the local root sig with their hit shader.
//    // Edit: External data will be obtained from a structured buffer, so unassociate local root signature from hit groups.
//    auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
//    localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());
//    // Shader association
//    auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
//    rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
//    //for (auto& hitGroup : c_hitGroupNames)
//    //    rootSignatureAssociation->AddExports(hitGroup);
//    //rootSignatureAssociation->AddExport(c_hitGroupNames[TriangleMeshes::Plane]);
//    rootSignatureAssociation->AddExport(c_raygenShaderName);
//}

// Create a raytracing pipeline state object (RTPSO).
// An RTPSO represents a full set of shaders reachable by a DispatchRays() call,
// with all configuration options resolved, such as local signatures and other state.
void Game::CreateRaytracingPipelineStateObject(
    ID3D12Device5* device,
    const unsigned char* shaderBytes, size_t byteLength, uint32_t payloadSize,
    ID3D12StateObject** stateObject)
//void Game::CreateRaytracingPipelineStateObject()
{
    //auto device = m_deviceResources->GetD3DDevice();

    // Create 7 subobjects that combine into a RTPSO:
    // Edit: Creating 8 subobjects.
    // Edit: Creating 10 subobjects.
    // Edit: Creating 14 subobjects.
    // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
    // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
    // This simple sample utilizes default shader association except for local root signature subobject
    // which has an explicit association specified purely for demonstration purposes.
    // 1 - DXIL library
    // 8 - (3 x cube + plane hit groups) x ray types
    // 1 - Shader config
    // 2 - 1 x Local root signature and associations
    // 1 - Global root signature
    // 1 - Pipeline config

    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    // DXIL library
    // This contains the shaders and their entrypoints for the state object.
    // Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
    auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

    // Replaced C style cast (void*) with C++ style cast.
    auto libdxil = CD3DX12_SHADER_BYTECODE(static_cast<const void*>(shaderBytes), byteLength);
    //auto libdxil = CD3DX12_SHADER_BYTECODE(static_cast<const void*>(precompiledShader), ARRAYSIZE(precompiledShader));

    lib->SetDXILLibrary(&libdxil);
    // Define which shader exports to surface from the library.
    // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
    // In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
    // Use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
    //{
    //    lib->DefineExport(c_raygenShaderName);
    //    for (UINT i = 0; i < TriangleMeshes::Count; i++)
    //    {
    //        lib->DefineExport(c_closestHitShaderNames[i]);
    //    }
    //    //lib->DefineExport(c_closestHitShaderName);
    //    lib->DefineExport(c_missShaderName);
    //}

   // Global root signature
   // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
   // Edit: The application is now using a global compute rootsig for all compute and DXR dispatches.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(m_rootSig[RootSignatures::Compute].Get());

    // Shader config
    // Defines the maximum sizes in bytes for the ray payload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();

    
    // Local root signature and shader association.
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    // To do: inline this function call.
    //CreateLocalRootSignatureSubobjects(&raytracingPipeline);

    // Edit: If local root arguments are not utilized by the application, we can dispense with LOCAL_ROOT_SIGNATURE and
    // SUBOBJECT_TO_EXPORTS_ASSOCIATION subobjects.

    /* <-- To include these optional subobjects, insert '//' in front of the ' /* ' at the beginning of this line.

    // Hit group and miss shaders in this sample are not using a local root signature and thus one is not associated with them.
    // Edit: Hit groups for cubes & plane require external data (material root constants) so associate the local root sig with their hit shader.
    // Edit: External data will be obtained from a structured buffer, so unassociate local root signature from hit groups.
    auto localRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    localRootSignature->SetRootSignature(m_rootSig[RootSignatures::DxrLocal].Get());
    // Shader association.
    auto rootSignatureAssociation = raytracingPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
    //for (auto& hitGroup : c_hitGroupNames)
    //    rootSignatureAssociation->AddExports(hitGroup);
    //rootSignatureAssociation->AddExport(c_hitGroupNames[TriangleMeshes::Plane]);
    rootSignatureAssociation->AddExport(c_raygenShaderName);
    // */

    // If there is more than one payload struct, get the size of the largest struct.
    // Edit: the MaxPayloadSizeInBytes field is ignored by drivers if PAQs are enabled (the default) in SM 6.7 or higher.
    // Edit: not able to successfully compile the RTPSO when compiling shaders with annotated payload structs!
    uint32_t attributeSize = 2 * sizeof(float); // float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);
    //shaderConfig->Config(0, attributeSize);
    //uint32_t payloadSize = static_cast<uint32_t>(std::max(sizeof(ColorRayPayload), sizeof(ShadowRayPayload)));
    //UINT payloadSize = 4 * sizeof(float);   // float4 color
    //shaderConfig->Config(Globals::MaxPayloadSize, attributeSize);
    //shaderConfig->Config(payloadSize, attributeSize);

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    // PERFOMANCE TIP: Set max recursion depth as low as needed as drivers may apply optimization strategies for low recursion depths. 
    uint32_t maxRecursionDepth = MAX_RAY_RECURSION_DEPTH;
    pipelineConfig->Config(maxRecursionDepth);

    // Get remaining required config from collection state object.
    auto collectionConfig = raytracingPipeline.CreateSubobject<CD3DX12_EXISTING_COLLECTION_SUBOBJECT>();
    collectionConfig->SetExistingCollection(m_stateObject[StateObjects::HitGroupCollection].Get());

#if _DEBUG
    PrintStateObjectDesc(raytracingPipeline);
#endif

    // Create the state object.
    ThrowIfFailed(device->CreateStateObject(
        raytracingPipeline,
        IID_PPV_ARGS(stateObject)),
        L"Couldn't create DirectX Raytracing state object.\n"
    );
    //ThrowIfFailed(m_dxrDevice->CreateStateObject(raytracingPipeline,
    // IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
}

// Build shader tables.
// This encapsulates all shader records - shaders and the arguments for their local root signatures.
void Game::BuildShaderTables(
    ID3D12Device5* device,
    ComPtr<ID3D12StateObject> stateObject,
    ShaderBindingTableGroup* sbtGroup)
{
    //auto device = m_deviceResources->GetD3DDevice();

    void* rayGenShaderID = {};
    void* missShaderIDs[RayType::Count] = {};
    //void* hitGroupShaderIDs[MeshType::Count] = {};
    void* hitGroupShaderIDs[MeshType::Count][RayType::Count] = {};
    //void* hitGroupShaderIDs[MeshInstances::Count][RayType::Count] = {};
    //void* hitGroupShaderIDs[m_geometryInstanceCount + 1][RayType::Count]{};
    //void* hitGroupShaderIdentifiers[TriangleMeshes::Count]{};
    //void* hitGroupShaderIdentifier{};

    // New.
    // A shader name look-up table for shader table debug print out.
    std::unordered_map<void*, std::wstring> shaderIdToStringMap;

    const auto GetShaderIDs = [&](auto* stateObjectProperties)
    {
        rayGenShaderID = stateObjectProperties->GetShaderIdentifier(c_raygenShaderName);
        shaderIdToStringMap[rayGenShaderID] = c_raygenShaderName;

        for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
        {
            missShaderIDs[rayType] = stateObjectProperties->GetShaderIdentifier(c_missShaderNames[rayType]);
            shaderIdToStringMap[missShaderIDs[rayType]] = c_missShaderNames[rayType];
        }
        for (uint32_t meshType = 0; meshType < MeshType::Count; meshType++)
            //for (uint32_t i = 0; i < MeshInstances::Count; i++)
            //for (UINT i = 0; i < m_geometryInstanceCount + 1; i++)
            //for (UINT i = 0; i < TriangleMeshes::Count; i++)
        {
            for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
            {
                hitGroupShaderIDs[meshType][rayType] = stateObjectProperties->GetShaderIdentifier(c_hitGroupNames[meshType][rayType]);
                shaderIdToStringMap[hitGroupShaderIDs[meshType][rayType]] = c_hitGroupNames[meshType][rayType];
                //hitGroupShaderIDs[meshType][rayType] = stateObjectProperties->GetShaderIdentifier(c_hitGroupNames[meshType][rayType]);
                //shaderIdToStringMap[hitGroupShaderIDs[meshType][rayType]] = c_hitGroupNames[meshType][rayType];
            }
        }
    };

    // Get shader identifiers.
    const uint32_t shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    {
        ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
        ThrowIfFailed(stateObject.As(&stateObjectProperties));
        GetShaderIDs(stateObjectProperties.Get());
        //shaderIDSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    }

    // Ray gen shader table
    {
        //struct RootArguments {
        //    RayGenConstantBuffer cb;
        //} rootArguments{};
        //rootArguments.cb = m_rayGenCB;

        const uint32_t numShaderRecords = 1;
        const uint32_t shaderRecordSize = shaderIDSize; // no root arguments
        //UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
        ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
        rayGenShaderTable.push_back(ShaderRecord(rayGenShaderID, shaderRecordSize));// , nullptr, 0));
        //rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
        rayGenShaderTable.DebugPrint(shaderIdToStringMap);
        sbtGroup->raygeneration.strideInBytes = 0;
        sbtGroup->raygeneration.buffer = rayGenShaderTable.GetResource();
        //m_rayGenShaderTable->strideInBytes = 0;
        //m_rayGenShaderTable->buffer = rayGenShaderTable.GetResource();
    }
    // Miss shader table
    {
        const uint32_t numShaderRecords = RayType::Count;
        const uint32_t shaderRecordSize = shaderIDSize; // no root arguments
        ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
        for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
        {
            missShaderTable.push_back(ShaderRecord(missShaderIDs[rayType], shaderIDSize));// , nullptr, 0));
        }
        missShaderTable.DebugPrint(shaderIdToStringMap);
        sbtGroup->miss.strideInBytes = missShaderTable.GetShaderRecordSize();
        sbtGroup->miss.buffer = missShaderTable.GetResource();
        //m_missShaderTable->strideInBytes = missShaderTable.GetShaderRecordSize();
        //m_missShaderTableStrideInBytes = missShaderTable.GetShaderRecordSize(); // was missing this!
        //m_missShaderTable->buffer = missShaderTable.GetResource();
    }
    // Hit group shader table
    {
        //struct RootArguments {
        //    CameraConstants cc;
        //} rootArguments{};
        //rootArguments.cc = m_cameraConstants;

        //uint32_t numShaderRecords = MeshType::Count;
        const uint32_t numShaderRecords = MeshType::Count * RayType::Count;
        const uint32_t shaderRecordSize = shaderIDSize;
        //uint32_t numShaderRecords = MeshInstances::Count * RayType::Count;
        //UINT numShaderRecords = (m_geometryInstanceCount + 1) * RayType::Count;
        //UINT numShaderRecords = TriangleMeshes::Count;// 1;
        //uint32_t shaderRecordSize = shaderIDSize + sizeof(ObjectConstants);
        //UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
        ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");

        //hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifiers[TriangleMeshes::Cube], shaderIdentifierSize));
        //hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifiers[TriangleMeshes::Plane], shaderIdentifierSize,
        // &m_materialConstants, sizeof(MaterialConstants)));
        //hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));

        // Create a shader record for each geometry.
        for (uint32_t meshType = 0; meshType < MeshType::Count; meshType++)
        //for (uint32_t instanceIndex = 0; instanceIndex < MeshInstances::Count; instanceIndex++)
            //for (UINT instanceIndex = 0; instanceIndex < m_geometryInstanceCount + 1; instanceIndex++)
        {
            for (uint32_t rayType = 0; rayType < RayType::Count; rayType++)
            {
                //auto& hitGroupShaderID = hitGroupShaderIDs[meshType];
                const auto& hitGroupShaderID = hitGroupShaderIDs[meshType][rayType];
                hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderID, shaderIDSize));

                //switch (instanceIndex)
                //{
                //case MeshInstances::RedCube :
                //case MeshInstances::GreenCube :
                //case MeshInstances::BlueCube :
                //    hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderID, shaderIDSize, &m_cubeConstants[instanceIndex],
                // sizeof(ObjectConstants)));
                //    break;
                //
                //case MeshInstances::Plane :
                //    hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderID, shaderIDSize, &(*m_planeConstants),
                // sizeof(ObjectConstants)));
                //    //hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderID, shaderIdentifierSize));
                //    break;
                //
                //default:
                //    break;
                //}
            }
        }
        hitGroupShaderTable.DebugPrint(shaderIdToStringMap); // added this
        // Hit group shader table stride will be larger than shaderID size if adding space for root constants.
        sbtGroup->hitgroup.strideInBytes = hitGroupShaderTable.GetShaderRecordSize();
        sbtGroup->hitgroup.buffer = hitGroupShaderTable.GetResource();
        //m_hitGroupShaderTable->strideInBytes = hitGroupShaderTable.GetShaderRecordSize();
        //m_hitGroupShaderTable->buffer = hitGroupShaderTable.GetResource();
    }
}

// Build acceleration structures needed for raytracing.
//void Game::BuildAccelerationStructures(
//    ID3D12Device5* device,
//    ID3D12GraphicsCommandList4* commandList,
//    ID3D12CommandAllocator* commandAlloc)
//{
    //auto device = m_deviceResources->GetD3DDevice();
    //auto commandList = m_deviceResources->GetCommandList();
    //auto commandAllocator = m_deviceResources->GetCommandAllocator();

    // Reset the command list for the acceleration structure construction.
    //commandList->Reset(commandAlloc, nullptr);

    // Build all bottom-level AS.
    //BuildBLASGeometryDescs();
    //BuildStaticBLAS(device, commandList);           // Static BLAS should only be built once and do not require updating.
    //BuildDynamicBLAS(device, commandList, false);   // This is the first dynamic BLAS build, after which, only updates are required.

    // Build top-level AS.
    //BuildTLAS(device, commandList, m_mainScene->GetTLASBuffers(), m_mainScene->GetTLASInstanceDesc(), SceneMain::TLASInstances::tlasCount, false); // First TLAS build.
    //BuildTLAS(device, commandList, m_mainScene->GetTLASBuffers(), false); // first TLAS build
    //BuildTLAS(device, commandList, false); // first TLAS build
    //BuildTopLevelAS(m_blasBuffers, false); // first TLAS build
    //BuildTopLevelAS(m_blasBuffers);
    //AccelerationStructureBuffers topLevelAS = BuildTopLevelAS(m_blasBuffers);
    //AccelerationStructureBuffers topLevelAS = BuildTopLevelAS(bottomLevelAS);

    // Kick off acceleration structure construction.
    //m_deviceResources->ExecuteCommandList();

    // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
    //m_deviceResources->WaitForGpu();

    // Store the AS buffers. The rest of the buffers will be released once we exit the function.
    //for (UINT i = 0; i < TriangleMeshes::Count /*GeometryType::Count*/; i++)
    //{
    //    m_bottomLevelAS[i] = m_blasBuffers[i].accelerationStructure;
    //    //m_bottomLevelAS[i] = bottomLevelAS[i].accelerationStructure;
    //}
    //m_topLevelAS = m_tlasBuffers.accelerationStructure;
    //m_topLevelAS = topLevelAS.accelerationStructure;
//}

/*
void Game::UpdateTopLevelAS(std::vector<AccelerationStructureBuffers>& bottomLevelAS)
//void Game::UpdateTopLevelAS(AccelerationStructureBuffers bottomLevelAS[TriangleMeshes::Count])
{
    auto commandList = m_deviceResources->GetCommandList();
    auto commandAllocator = m_deviceResources->GetCommandAllocator();

    // Reset the command list for the acceleration structure construction.
    commandList->Reset(commandAllocator, nullptr);

    //auto& topLevelBuildDesc = m_tlasBuffers.m_topLevelBuildDesc;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    //D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelBuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelBuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelBuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    topLevelBuildDesc.Inputs.NumDescs = m_geometryInstanceCount + 1; // three triangle instances + ground plane

    // Update instance descs for the bottom-level acceleration structures.
    {
        // Not referenced.
        //D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[GeometryType::Count] = {};
        std::vector<D3D12_GPU_VIRTUAL_ADDRESS> bottomLevelASaddresses =
        //D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASaddresses[TriangleMeshes::Count] =
        //D3D12_GPU_VIRTUAL_ADDRESS bottomLevelASaddresses[GeometryType::Count] =
        {
            bottomLevelAS[TriangleMeshes::Cube].accelerationStructure->GetGPUVirtualAddress(),
            bottomLevelAS[TriangleMeshes::Plane].accelerationStructure->GetGPUVirtualAddress()
        };
        BuildBottomLevelASInstanceDescs(bottomLevelASaddresses, &m_tlasBuffers.instanceDesc); // Must pass a pointer to pointer.
    }

    // Update top-level AS desc with new instance data
    // Edit:GPU virtual address of instance desc resource doesn't change after update,
    // but commenting out this assignment may be unsafe.
    //topLevelBuildDesc.Inputs.InstanceDescs = m_tlasBuffers.instanceDesc->GetGPUVirtualAddress();

    topLevelBuildDesc.DestAccelerationStructureData = m_tlasBuffers.accelerationStructure->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = m_tlasBuffers.scratch->GetGPUVirtualAddress();
    topLevelBuildDesc.Inputs.InstanceDescs = m_tlasBuffers.instanceDesc->GetGPUVirtualAddress();

    // Build acceleration structure.
    commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    // Kick off acceleration structure construction.
    m_deviceResources->ExecuteCommandList();
    m_deviceResources->WaitForGpu();
}
*/
