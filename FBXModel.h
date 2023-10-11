//***************************************************************************************
// FBXModel.h by Maico De Blasio (C) 2021 All Rights Reserved.
// This class loads and manages skinned models saved to the FBX file format.
//***************************************************************************************

#pragma once

// AutodeskMemoryStream fails FBX file load in VS2022 17.4 Release build.
//#include "AutodeskMemoryStream.h"

// To test:
// Thurman states the bone matrix size should be at least:
// vertices per triangle * bone influences per vertex

class FBXModel
{
public:

    FBXModel(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const char* pFbxFilePath) noexcept;
    //FbxLoader(const char* pFbxFilePath) noexcept;
    ~FBXModel(); // implemented

    FBXModel(FBXModel const&) = delete;
    FBXModel& operator= (FBXModel const&) = delete;

    FBXModel(FBXModel&&) = default;
    FBXModel& operator= (FBXModel&&) = default;

    //FbxLoader(CONST FbxLoader& rhs);

    static const uint32_t MaxBones = 50;

private:

    struct Vertex
    {
        // Added constructors.
        Vertex() = default;

        Vertex(
            float x, float y, float z,
            float nx,float ny,float nz,
            float u, float v,
            float tx,float ty,float tz) :
            pos(x, y, z),
            normal(nx, ny, nz),
            texC(u, v),
            tangent(tx, ty, tz) {}

        Vertex(
            DirectX::SimpleMath::Vector3 pos,
            DirectX::SimpleMath::Vector3 normal,
            DirectX::SimpleMath::Vector2 texC,
            DirectX::SimpleMath::Vector3 tangent) :
            pos(pos), normal(normal), texC(texC), tangent(tangent) {}

        DirectX::SimpleMath::Vector3 pos;
        DirectX::SimpleMath::Vector3 normal;
        DirectX::SimpleMath::Vector2 texC;
        DirectX::SimpleMath::Vector3 tangent;
        DirectX::SimpleMath::Vector4 boneWeights;
        //float boneWeights[4];
        uint8_t boneIndices[4];
        //UINT BoneIndices[4u];
    };
    
    // This new vertex struct will be used to store the results of skinning in the compute shader.
    struct SkinnedVertex
    {
        // Added constructors.
        SkinnedVertex() = default;

        SkinnedVertex(
            float x, float y, float z,
            float nx, float ny, float nz,
            float u, float v,
            float tx, float ty, float tz) :
            pos(x, y, z),
            normal(nx, ny, nz),
            texC(u, v),
            tangent(tx, ty, tz) {}

        SkinnedVertex(
            DirectX::SimpleMath::Vector3 pos,
            DirectX::SimpleMath::Vector3 normal,
            DirectX::SimpleMath::Vector2 texC,
            DirectX::SimpleMath::Vector3 tangent) :
            pos(pos), normal(normal), texC(texC), tangent(tangent) {}

        DirectX::SimpleMath::Vector3 pos;
        DirectX::SimpleMath::Vector3 normal;
        DirectX::SimpleMath::Vector2 texC;
        DirectX::SimpleMath::Vector3 tangent;
    };

    struct IndexWeightPair
    {
        uint32_t boneWeights;
        uint32_t boneIndices;
    };

    struct BlendingIndexWeightPair
    {
        uint32_t blendingIndex;
        float    blendingWeight;

        BlendingIndexWeightPair() :
            blendingIndex(0),
            blendingWeight(0)
        {}
    };

    struct CtrlPoint
    {
        DirectX::SimpleMath::Vector3 position;
        std::vector<BlendingIndexWeightPair> blendingInfo;

        CtrlPoint()
        {
            blendingInfo.reserve(4);
        }
    };

    typedef std::vector<IndexWeightPair>::iterator tSkinnedVerticeIterator;
    typedef std::vector<IndexWeightPair>::const_iterator tSkinnedVerticeConstIterator;

    //typedef std::vector<IndexWeightPair> tSkinnedVerticeVector;
    //typedef std::vector<Vertex> tSkinnedVerticeVector;
    //typedef tSkinnedVerticeVector::iterator tSkinnedVerticeIterator;
    //typedef tSkinnedVerticeVector::const_iterator tSkinnedVerticeConstIterator;

    typedef std::vector<uint16_t>::iterator tVertexIndexIterator;
    typedef std::vector<uint16_t>::const_iterator tVertexIndexConstIterator;

    //typedef std::vector<USHORT> tVertexIndexVector;
    //typedef tVertexIndexVector::iterator tVertexIndexIterator;
    //typedef tVertexIndexVector::const_iterator tVertexIndexConstIterator;

    struct Mesh
    {
        Mesh();
        ~Mesh();

        //CD3DX12_RESOURCE_DESC vertexBufferResourceDesc;
        //CD3DX12_RESOURCE_DESC indexBufferResourceDesc;

        //CD3DX12_HEAP_PROPERTIES heapProperties;

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
        D3D12_VERTEX_BUFFER_VIEW skinnedVertexBufferView;
        D3D12_INDEX_BUFFER_VIEW  indexBufferView;

        DirectX::BoundingBox    boundingBox;
        DirectX::BoundingSphere boundingSphere;

        //DirectX::SharedGraphicsResource vertexBufferSharedResource;
        //DirectX::SharedGraphicsResource indexBufferSharedResource;

        Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> skinnedVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

        std::unordered_map<uint32_t, CtrlPoint*> controlPoints;
        std::string materialName;
        std::string meshName;

        std::vector<Vertex>   vertices;
        std::vector<SkinnedVertex> skinnedVertices;
        std::vector<uint32_t> indices;
        //std::vector<uint16_t> indices;

        std::vector<Vertex>   finalVertices;
        std::vector<SkinnedVertex> finalSkinnedVertices;
        std::vector<uint16_t> finalIndices16;
        std::vector<uint32_t> finalIndices32;

        uint32_t vertexBufferSize;
        uint32_t skinnedVertexBufferSize;
        uint32_t indexBufferSize;
        uint32_t numIndices;
        uint32_t numVertices;

        std::vector<IndexWeightPair> verticeVector;
        std::vector<uint16_t>        indexVector;
        //tSkinnedVerticeVector mVerticeVector;
        //tVertexIndexVector mIndexVector;

        //VOID UploadMesh(CONST std::vector<SkinnedVertex>& vertices, CONST std::vector<USHORT>& indices);

        //Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer() { return mVertexBuffer; }
        //Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer() { return mIndexBuffer; }
        //
        //DirectX::SharedGraphicsResource VBSharedResource() { return mSharedResourceVB; }
        //DirectX::SharedGraphicsResource IBSharedResource() { return mSharedResourceIB; }
        //
        //D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return mVertexBufferView; }
        //D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return mIndexBufferView; }
        //
        //UINT GetIndexCount() { return mNumIndices; }
    };

    // This is a stupidly simple way to slice and dice bytes
    // and words. This is great for grabbing color channels
    // and bone influences.
    union PackedInt
    {
        uint8_t  bytes[4];
        uint32_t number;
    };

    // This stores the information of each key frame of each joint
    // This is a linked list and each node is a snapshot of the
    // global transformation of the joint at a certain frame
    struct Keyframe
    {
        FbxLongLong frameNum;
        FbxAMatrix  globalTransform;
        Keyframe*   next;

        Keyframe() :
            next(nullptr) {}
    };

    struct Bone
    {
        std::string name;

        // calculates the locals together depth first
        // equals localTransform * parentMatrix
        DirectX::SimpleMath::Matrix combinedTransform;

        // updated for animation
        DirectX::SimpleMath::Matrix nodeLocalTransform;

        // This is an inverse of the parent to child matrix,
        // or you can call it the inverse bind pose matrix.
        // It's set at mesh load time and doesn't change.
        DirectX::SimpleMath::Matrix offset;

        // Ready for a skinned mesh shader as the bone matrice.
        // equals offset * combinedTransform
        DirectX::SimpleMath::Matrix boneMatrice;
        FbxNode*     boneNodePtr;
        FbxSkeleton* fbxSkeletonPtr;
        FbxCluster*  fbxClusterPtr;

        // Relationship Stuff:
        std::vector<int> childIndexes;
        int parentIndex;
    };

    // declare global
    FbxManager*                  m_sdkManager;
    FbxScene*                    m_scene;

    static FbxLongLong           m_animationLength;
    static std::string           m_animationName;

    ID3D12Device*                m_d3dDevice;
    ID3D12CommandQueue*          m_commandQueue;

    DirectX::SimpleMath::Vector3 m_position;
    DirectX::SimpleMath::Vector3 m_orientation; // X, Y, Z rotations in radians.
    DirectX::SimpleMath::Matrix  m_world;

    std::unique_ptr<DirectX::XMFLOAT3X4[]> m_bonePalette3X4; // final bone matrix for shader consumption
    //DirectX::XMFLOAT3X4* mBonePalette3X4;

    // Used to do a reverse lookup.
    typedef std::vector<uint32_t> tControlPointIndexes;
    typedef std::vector<tControlPointIndexes> tControlPointRemap;

    std::vector<Bone>            m_boneVector;
    std::vector<Mesh>            m_meshes;

    std::vector<DirectX::SimpleMath::Matrix> m_bonePalette; // final bone matrix

    size_t                       m_initialAnimDuration_ms;

private:

    // To read a file using an FBX SDK reader.
    bool LoadScene(FbxManager* pSdkManager, FbxScene* pScene, const char* pFbxFilePath);

    // to create a SDK manager and a new scene
    void InitializeSdkManagerAndScene();

    // to build a scene from an FBX file
    bool LoadFBXScene(const char* pFbxFilePath);

    // to destroy an instance of the SDK manager
    void DestroySdkObjects(FbxManager* pSdkManager);

    // to get the root node
    const FbxNode* GetRootNode();

    // to get the root node name
    const char* GetRootNodeName();

    // to get a string from the node default translation values
    FbxString GetDefaultTranslationInfo(const FbxNode* pNode);

    // to get a string from the node name and attribute type
    FbxString GetNodeNameAndAttributeTypeName(const FbxNode* pNode);

    // to get a string from the node visibility value
    FbxString GetNodeVisibility(const FbxNode* pNode);

    bool IsMeshSkinned(FbxMesh* pMesh);

    int BoneNameToIndex(const std::string& boneName);
    int GetBoneCount(FbxMesh* pMesh);

    size_t GetAnimationDuration();

    int FindVertex(const std::vector<Vertex>& finalVertices, const Vertex& v);
    //uint16_t FindVertex(const std::vector<Vertex>& finalVertices, const Vertex& v);
    void AddBoneInfluence(std::vector<IndexWeightPair>& skinnedVerticeVector, int vertexIndex, int boneIndex, float boneWeight);
    //VOID AddBoneInfluence(tSkinnedVerticeVector& skinnedVerticeVector, UINT vertexIndex, UINT boneIndex, FLOAT boneWeight);
    void BuildMatrices(const FbxTime& frameTime);
    void CopyBoneWeightsToVertex(Mesh& mesh);
    void CopyBoneWeightsToVertex(FbxMesh* pMesh, Mesh& mesh);
    void DedupeVertices(Mesh& mesh);
    void FbxToMatrix(const FbxAMatrix& fbxMatrix, DirectX::SimpleMath::Matrix& matrix);
    void GetGeometryTransformMatrix(FbxNode* pNode, FbxAMatrix& offsetMatrix);
    //VOID GetGeometryTransformMatrix(FbxNode* pNode, DirectX::SimpleMath::Matrix& offsetMatrix);
    void GetNodeLocalTransform(FbxNode* pNode, DirectX::SimpleMath::Matrix& matrix);
    void GetNodeLocalTransform(FbxNode* pNode, const FbxTime& fbxTime, DirectX::SimpleMath::Matrix& matrix);
    void LoadControlPointRemap(FbxMesh* pMesh, tControlPointRemap& controlPointRemap);
    void LoadControlPoints(FbxMesh* pMesh, Mesh& mesh);
    void LoadNodeLocalTransformMatrices(const FbxTime& frameTime);
    void LoadNormalTexTangent(FbxMesh* pMesh, Mesh& mesh);
    void LoadBone(FbxNode* pNode, int parentBoneIndex);
    void LoadBones(FbxNode* pNode, int parentBoneIndex);
    void LoadMeshes(FbxNode* pNode);
    void LoadMesh(FbxNode* pNode);
    void LoadMeshBoneWeightsIndices(FbxMesh* pMesh, Mesh& mesh);
    //VOID LoadMeshBoneWeightsIndices(FbxNode* pNode, Mesh& mesh);
    void NormalizeBoneWeights(Mesh& mesh);
    void ReadNormal(FbxMesh* pMesh, int cpIndex, int vCounter, DirectX::SimpleMath::Vector3& n);
    void ReadTexCoord(FbxMesh* pMesh, int cpIndex, int uvIndex, DirectX::SimpleMath::Vector2& t);
    void ReadTangent(FbxMesh* pMesh, int cpIndex, int vCounter, DirectX::SimpleMath::Vector3& u);

    void CalculateCombinedTransforms();
    void CalculatePaletteMatrices();
    void CreateMeshBoundingBoxes();
    void CreateMeshBoundingSpheres();
    void UploadMeshes(); // populate DirectX vertex buffer & index buffer resources
    //void CreateBufferResources(Mesh& mesh);

public:

    //auto& GetMeshes() noexcept { return mMeshes; } // non-const because meshes will be modified by caller
    //CONST auto GetMeshCount() CONST noexcept { return mMeshes.size(); }

    // Iterator interface
    //auto begin() { return m_meshes.begin(); }
    //auto end()   { return m_meshes.end(); }

    const auto GetVertexBuffer(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.vertexBuffer.Get();
    }

    const auto GetSkinnedVertexBuffer(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.skinnedVertexBuffer.Get();
    }

    const auto GetIndexBuffer(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.indexBuffer.Get();
    }

    const auto GetIndexCount(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.numIndices;
    }

    const auto GetIndexFormat(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.indexBufferView.Format;
    }

    const auto GetVertexCount(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.numVertices;
    }

    const auto GetVertexStride() const noexcept                     { return static_cast<uint32_t>(sizeof(Vertex)); }
    const auto GetSkinnedVertexStride() const noexcept              { return static_cast<uint32_t>(sizeof(SkinnedVertex)); }

    //const auto  GetVertexBuffer(Mesh& mesh) const noexcept        { return mesh.vertexBuffer.Get(); }
    //const auto  GetIndexBuffer(Mesh& mesh) const noexcept         { return mesh.indexBuffer.Get(); }

    //const auto& GetVertexBufferMemory(Mesh& mesh) const noexcept    { return mesh.vertexBufferSharedResource; }
    //const auto& GetIndexBufferMemory(Mesh& mesh) const noexcept     { return mesh.indexBufferSharedResource; }

    //CONST auto GetVertexBuffer(UINT index) CONST noexcept { return mMeshes[index].mVertexBuffer.Get(); }
    //CONST auto GetIndexBuffer(UINT index) CONST noexcept { return mMeshes[index].mIndexBuffer.Get(); }
    //
    //CONST auto& GetVBMemory(UINT index) CONST noexcept { return mMeshes[index].mSharedResourceVB; }
    //CONST auto& GetIBMemory(UINT index) CONST noexcept { return mMeshes[index].mSharedResourceIB; }

    const auto GetBoundingBox(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.boundingBox;
        //return mMeshes[index].mBoundingBox;
    }

    const auto GetBoundingSphere(size_t pos) const noexcept
    {
        auto& mesh = m_meshes.at(pos);
        return mesh.boundingSphere;
        //return mMeshes[index].mBoundingSphere;
    }
    //const auto GetBoundingBox(Mesh& mesh) const noexcept           { return mesh.boundingBox; }
    //const auto GetBoundingSphere(Mesh& mesh) const noexcept        { return mesh.boundingSphere; }

    const auto  GetPosition() const noexcept                        { return m_position; }
    const auto  GetOrientation() const noexcept                     { return m_orientation; }
    const auto  GetWorld() const noexcept                           { return m_world; }

    const auto  GetAnimDuration() const noexcept                    { return m_initialAnimDuration_ms; } // Debugging.

    void AdvanceTime(float time);
    //void CreateBufferResources(ID3D12Device* device, Mesh& mesh);
    //VOID CreateBufferResources(ID3D12Device* device, UINT index);

    //void CreateBufferViews(Mesh& mesh);
    //VOID CreateBufferViews(UINT index);

    //void SetPosition(DirectX::SimpleMath::Vector3 const& pos) { m_position = pos; }

    void SetWorld(DirectX::SimpleMath::Vector3 const& pos,
                  DirectX::SimpleMath::Vector3 const& orient) noexcept
    {
        m_position    = pos;
        m_orientation = orient;
        m_world       = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(orient) *
                        DirectX::SimpleMath::Matrix::CreateTranslation(pos);
    }
    //void SetWorld(DirectX::SimpleMath::Matrix const& world)   { m_world = world; }

    // This function returns to the caller a XMFLOAT3X4 pointer to the bone palette, but we still have to dereference
    // the pointer in a loop within the caller before uploading to a cbuffer!

    //DirectX::XMFLOAT3X4* GetMatrixPalette() CONST
    //{
    //    DirectX::XMFLOAT3X4 boneTransforms[MAX_BONES]{};
    //    for (UINT i = 0u; i < mBonePalette.size(); i++)
    //    {
    //        // We do not transpose bone matrices for HLSL because the original
    //        // FbxMatrix is in column-vector math format.
    //        // XMFLOAT3X4 matrices are also column-major, so no transpose for HLSL neccessary.
    //        XMStoreFloat3x4(&boneTransforms[i], mBonePalette[i]);
    //    }
    //    return boneTransforms;
    //}

    //VOID GetMatrixPalette(std::vector<DirectX::SimpleMath::Matrix>& bonePalette) CONST
    //{
    //   bonePalette = mBonePalette;
    //}
    //CONST std::vector<DirectX::SimpleMath::Matrix>& GetMatrixPalette() { return mBonePalette; }

    const auto GetBonePaletteSize() const { return static_cast<uint32_t>(m_bonePalette.size() * sizeof(DirectX::XMFLOAT3X4)); }
    const auto GetBonePalette3X4()  const { return m_bonePalette3X4.get(); }

    void Draw(ID3D12GraphicsCommandList* commandList);
    void DrawSkinned(ID3D12GraphicsCommandList* commandList);

    //DirectX::XMFLOAT3X4 mBonePalette3X4[MAX_BONES]; // final bone matrix for shader consumption
};
