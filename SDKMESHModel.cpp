//***************************************************************************************
// SDKMESHModel.cpp by Maico De Blasio (C) 2021 All Rights Reserved.
//***************************************************************************************

#include "pch.h"
#include "SDKMESHModel.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DX;

SDKMESHModel::SDKMESHModel(
    ID3D12Device* device,
    ID3D12CommandQueue* commandQueue,
    const wchar_t* renderingFile,
    const wchar_t* collisionFile) noexcept :
   m_d3dDevice(device), m_commandQueue(commandQueue)
//StaticModel::StaticModel(ID3D12Device* device, const wchar_t* renderingFile, const wchar_t* collisionFile) noexcept
{
    m_renderingModel = Model::CreateFromSDKMESH(device, renderingFile);
    m_collisionModel = Model::CreateFromSDKMESH(device, collisionFile);

    UploadModels(); // Currently just uploads the rendering model.
}

void SDKMESHModel::UploadModels()
//void StaticModel::GetModelData(DirectX::Model const& model)
{
    ResourceUploadBatch resourceUpload(m_d3dDevice);
    resourceUpload.Begin();

    m_renderingModel->LoadStaticBuffers(m_d3dDevice, resourceUpload, true);
    m_collisionModel->LoadStaticBuffers(m_d3dDevice, resourceUpload, true);

    auto uploadResourcesFinished = resourceUpload.End(m_commandQueue);
    uploadResourcesFinished.wait();

    // Populate member collision data.
    auto& modelMesh = m_collisionModel->meshes.at(0);
    auto& meshPart  = modelMesh->opaqueMeshParts.at(0);

    m_vertices    = reinterpret_cast<Vertex*>(meshPart->vertexBuffer.Memory());
    m_indices16   = reinterpret_cast<uint16_t*>(meshPart->indexBuffer.Memory());
    m_indices32   = reinterpret_cast<uint32_t*>(meshPart->indexBuffer.Memory());
    m_indexFormat = meshPart->indexFormat;
    m_triangles   = meshPart->indexCount / 3;

    //OptimizeMesh();
}
