//***************************************************************************************
// SDKMESHModel.h by Maico De Blasio (C) 2021 All Rights Reserved.
// This class loads and manages static models saved to the SDKMESH file format.
//***************************************************************************************

#pragma once

class SDKMESHModel
{
public:

    SDKMESHModel(
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        const wchar_t* renderingFile,
        const wchar_t* collisionFile) noexcept;
    //StaticModel(ID3D12Device* device, const wchar_t* renderingFile, const wchar_t* collisionFile) noexcept;

    SDKMESHModel(const SDKMESHModel& rhs) = delete;
    SDKMESHModel& operator=(const SDKMESHModel& rhs) = delete;

    SDKMESHModel(SDKMESHModel&&) = default;
    SDKMESHModel& operator=(SDKMESHModel&&) = default;

    ~SDKMESHModel() = default;

    // Vertex structure to support ray-triangle collision test.
    // Since we are loading SDKMESH model data, we have to define a vertex structure matching that is
    // exported by the SDKMESH exporter tool.
    // Vertex structure data was output from the tool (using -verbose option) as follows:
    // Warning: An incorrect vertex structure will result in an incorrect vertex stride value used in the
    // ray-triangle intersection algorithm.

    // Element  0 Stream  0 Offset  0 : Position.0  Type Float3(12 bytes)
    // Element  1 Stream  0 Offset 12 :   Normal.0  Type Float3(12 bytes)
    // Element  2 Stream  0 Offset 24 : TexCoord.0  Type Float2(8 bytes)
    // Element  3 Stream  0 Offset 32 :  Tangent.0  Type Float3(12 bytes)
    // Element  4 Stream  0 Offset 44 : Binormal.0  Type Float3(12 bytes)


private:

    //StaticModel(CONST StaticModel& rhs);

    struct Vertex
    {
        DirectX::SimpleMath::Vector3 Position;
        DirectX::SimpleMath::Vector3 Normal;
        DirectX::SimpleMath::Vector2 TexCoord;
        DirectX::SimpleMath::Vector3 Tangent;
        //DirectX::SimpleMath::Vector3 Binormal;
    };

    std::unique_ptr<DirectX::Model> m_renderingModel;
    std::unique_ptr<DirectX::Model> m_collisionModel;

    //DirectX::ModelMesh* mMesh;
    //DirectX::ModelMeshPart* mMeshPart;

    DirectX::SimpleMath::Vector3 m_position;
    DirectX::SimpleMath::Vector3 m_orientation; // X, Y, Z rotations in radians.
    DirectX::SimpleMath::Vector3 m_forward;     // Forward vector.
    DirectX::SimpleMath::Vector3 m_up;          // Up vector.
    DirectX::SimpleMath::Matrix  m_world;

    // Cached collision model parameters.
    DXGI_FORMAT m_indexFormat;
    Vertex*     m_vertices;
    uint16_t*   m_indices16;
    uint32_t*   m_indices32;
    uint32_t    m_triangles;

    ID3D12Device*       m_d3dDevice;
    ID3D12CommandQueue* m_commandQueue;

private:
    void UploadModels();
    //VOID OptimizeMesh();

public:
    //void GetModelData(DirectX::Model const& model);

    //
    // Rendering model accessors.
    //

    const auto GetVertexBuffer(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart  = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->staticVertexBuffer.Get();
        //return meshPart->vertexBuffer.Resource();
    }

    const auto GetIndexBuffer(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart  = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->staticIndexBuffer.Get();
        //return meshPart->indexBuffer.Resource();
    }

    const auto GetIndexBufferSize(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->indexBufferSize;
    }

    const auto GetIndexCount(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart  = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->indexCount;
        //return meshPart->indexBufferSize / static_cast<uint32_t>(sizeof(uint16_t));
    }

    //const auto GetIndexStart(size_t meshPos, size_t meshPartPos) const noexcept
    //{
    //    auto& modelMesh = m_renderingModel->meshes.at(meshPos);
    //    auto& meshPart = modelMesh->opaqueMeshParts.at(meshPartPos);
    //    return meshPart->startIndex;
    //}

    const auto GetIndexFormat(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart  = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->indexFormat;
    }

    //const auto IsIndex16(size_t meshPos, size_t meshPartPos) const noexcept
    //{
    //    auto& modelMesh = m_renderingModel->meshes.at(meshPos);
    //    auto& meshPart = modelMesh->opaqueMeshParts.at(meshPartPos);
    //    return meshPart->indexCount < UINT16_MAX;
    //}

    const auto GetVertexCount(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart  = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->vertexCount;
    }

    const auto GetVertexStride(size_t meshPos, size_t meshPartPos) const noexcept
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart  = modelMesh->opaqueMeshParts.at(meshPartPos);
        return meshPart->vertexStride;
    }

    //void SetPosition(DirectX::SimpleMath::Vector3 const& pos) { m_position = pos; }
    void SetWorld(DirectX::SimpleMath::Vector3 const& pos,
                  DirectX::SimpleMath::Vector3 const& orient) noexcept
    {
        m_position    = pos;
        m_orientation = orient;
        m_world       = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(orient) *
                        DirectX::SimpleMath::Matrix::CreateTranslation(pos);
    }
    void SetWorld(DirectX::SimpleMath::Vector3 const& pos,
                  DirectX::SimpleMath::Vector3 const& forward,
                  DirectX::SimpleMath::Vector3 const& up) noexcept
    {
        m_position = pos;
        m_forward  = forward;
        m_up       = up;
        m_world    = CreateWorld(pos, forward, up);
        //m_world    = DirectX::SimpleMath::Matrix::CreateWorld(pos, forward, up);
    }

    // Temp replacement method to substitute for SimpleMath method that may be calculating a LHS world matrix.
    // This replacement should calculate a RHS world matrix.
    DirectX::SimpleMath::Matrix CreateWorld(
        const DirectX::SimpleMath::Vector3& position,
        const DirectX::SimpleMath::Vector3& forward,
        const DirectX::SimpleMath::Vector3& up) noexcept
    {
        using namespace DirectX;
        const XMVECTOR zaxis = XMVector3Normalize(XMLoadFloat3(&forward));
        //const XMVECTOR zaxis = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(&forward)));
        XMVECTOR yaxis = XMLoadFloat3(&up);
        const XMVECTOR xaxis = XMVector3Normalize(XMVector3Cross(yaxis, zaxis));
        yaxis = XMVector3Cross(zaxis, xaxis);

        SimpleMath::Matrix R;
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._11), xaxis);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._21), yaxis);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&R._31), zaxis);
        R._14 = R._24 = R._34 = 0.f;
        R._41 = position.x; R._42 = position.y; R._43 = position.z;
        R._44 = 1.f;
        return R;
    }

    const auto  GetPosition() const noexcept        { return m_position; }
    const auto  GetOrientation() const noexcept     { return m_orientation; }
    const auto  GetForward() const noexcept         { return m_forward; }
    const auto  GetUp() const noexcept              { return m_up; }
    const auto  GetWorld() const noexcept           { return m_world; }

    void Draw(ID3D12GraphicsCommandList* cmdList)
    {
        m_renderingModel->Draw(cmdList);
    }

    void Draw(size_t meshPos, size_t meshPartPos, ID3D12GraphicsCommandList* cmdList)
    {
        auto& modelMesh = m_renderingModel->meshes.at(meshPos);
        auto& meshPart = modelMesh->opaqueMeshParts.at(meshPartPos);
        meshPart->Draw(cmdList);
    }

    // Returns raw pointer to model for read only usage by caller.
    //const auto GetRenderingModel() const            { return m_renderingModel.get(); }

    //
    // Collision model accessors (collision models should only contain one mesh with one meshpart).
    //

    const auto GetCollisionVertices() const noexcept
    {
        //auto& modelMesh = m_collisionModel->meshes.at(0);
        //auto& meshPart  = modelMesh->opaqueMeshParts.at(0);
        //return reinterpret_cast<Vertex*>(meshPart->vertexBuffer.Memory());
        return m_vertices;
    }

    const auto GetCollisionIndexFormat() const noexcept
    {
        //auto& modelMesh = m_collisionModel->meshes.at(0);
        //auto& meshPart  = modelMesh->opaqueMeshParts.at(0);
        //return meshPart->indexFormat;
        return m_indexFormat;
    }

    const auto GetCollisionIndices16() const noexcept
    {
        //auto& modelMesh = m_collisionModel->meshes.at(0);
        //auto& meshPart  = modelMesh->opaqueMeshParts.at(0);
        //return meshPart->indexBuffer.Memory();
        return m_indices16;
    }
    const auto GetCollisionIndices32() const noexcept
    {
        return m_indices32;
    }

    const auto GetCollisionTriangleCount() const noexcept
    {
        //auto& modelMesh = m_collisionModel->meshes.at(0);
        //auto& meshPart  = modelMesh->opaqueMeshParts.at(0);
        //return meshPart->indexCount;
        return m_triangles;
    }

    const auto GetBoundingBox(size_t meshPos) const noexcept
    {
        auto& modelMesh = m_collisionModel->meshes.at(meshPos);
        return modelMesh->boundingBox;
    }

    const auto GetBoundingSphere(size_t meshPos) const noexcept
    {
        auto& modelMesh = m_collisionModel->meshes.at(meshPos);
        return modelMesh->boundingSphere;
    }

    // Returns raw pointer to model for read only usage by caller.
    //const auto GetCollisionModel() const            { return m_collisionModel.get(); }

    //VOID SetModel(std::unique_ptr<DirectX::Model> m) { model = std::move(m); }
    //VOID SetCollisionModel(std::unique_ptr<DirectX::Model> m) { collisionModel = std::move(m); }

    //VOID ResetModel() { model.reset(); }
    //VOID ResetCollisionModel() { collisionModel.reset(); }
};
