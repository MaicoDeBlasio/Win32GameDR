//***************************************************************************************
// FBXModel.cpp by Maico De Blasio (C) 2021 All Rights Reserved.
//***************************************************************************************

#include "pch.h"
#include "FBXModel.h"

#ifdef  IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pSdkManager->GetIOSettings()))
#endif

using namespace DirectX::SimpleMath;
using namespace DirectX;
using namespace DX;

template<class T> constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    assert(!(hi < lo));
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

FBXModel::FBXModel(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const char* pFbxFilePath) noexcept :
//FbxLoader::FbxLoader(const char* pFbxFilePath) noexcept :
  m_d3dDevice(device),  m_commandQueue(commandQueue), m_initialAnimDuration_ms(0)
{
    InitializeSdkManagerAndScene();
    LoadFBXScene(pFbxFilePath);
}

FBXModel::~FBXModel()
{
    DestroySdkObjects(m_sdkManager);
}

FBXModel::Mesh::Mesh() : numIndices(0), vertexBufferSize(0), indexBufferSize(0) {}

FBXModel::Mesh::~Mesh(){}

bool FBXModel::LoadFBXScene(const char* pFbxFilePath)
{
    // Load the scene.
    if (LoadScene(m_sdkManager, m_scene, pFbxFilePath) == false)
        return false;

    // Initialize 3X4 packed bone palette smart pointer.
    m_bonePalette3X4 = std::make_unique<DirectX::XMFLOAT3X4[]>(MaxBones);

    FbxNode* pRootNode = m_scene->GetRootNode();

    if (pRootNode == nullptr)
        return false;

    LoadBones(pRootNode, -1);
    LoadMeshes(pRootNode);

    UploadMeshes();
    //CreateMeshBoundingBoxes();
    //CreateMeshBoundingSpheres();

    // We just need the duration of the first animation track.
    m_initialAnimDuration_ms = GetAnimationDuration();

    return true;
}

// Creates an instance of the SDK manager
// and use the SDK manager to create a new scene
void FBXModel::InitializeSdkManagerAndScene()
{
    // Create the FBX SDK memory manager object.
    // The SDK Manager allocates and frees memory
    // for almost all the classes in the SDK.
    m_sdkManager = FbxManager::Create();

    // create an IOSettings object
    FbxIOSettings* ios = FbxIOSettings::Create(m_sdkManager, IOSROOT);
    m_sdkManager->SetIOSettings(ios);

    m_scene = FbxScene::Create(m_sdkManager, "");
}

// to destroy an instance of the SDK manager
void FBXModel::DestroySdkObjects(FbxManager* pSdkManager)
{
    // Delete the FBX SDK manager. All the objects that have been allocated 
    // using the FBX SDK manager and that haven't been explicitly destroyed 
    // are automatically destroyed at the same time.
    if (pSdkManager) pSdkManager->Destroy();
    //if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

// to read a file using an FBXSDK reader
bool FBXModel::LoadScene(FbxManager* pSdkManager, FbxScene* pScene, const char* pFbxFilePath)
{
    bool lStatus;

    // Create a memory buffer to store file
    std::ifstream file(pFbxFilePath, std::ios::binary | std::ios::ate);
    auto fileSize = static_cast<int>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer;
    buffer.resize(fileSize, 0);
    assert(file.read((char*)buffer.data(), fileSize));

    FbxIOPluginRegistry* fbxIoPluginRegistry = pSdkManager->GetIOPluginRegistry();
    //tAutodeskMemoryStream stream = tAutodeskMemoryStream((CHAR*)buffer.data(), fileSize,
    //    fbxIoPluginRegistry->FindReaderIDByExtension("fbx"));

    // Create an importer.
    FbxImporter* lImporter = FbxImporter::Create(pSdkManager, "");

    // Initialize the importer by providing a filename.
    //auto lImportStatus = lImporter->Initialize(&stream, nullptr, -1, pSdkManager->GetIOSettings());
    bool lImportStatus = lImporter->Initialize(pFbxFilePath, -1, pSdkManager->GetIOSettings());

    if (!lImportStatus)
    {
        // Destroy the importer
        lImporter->Destroy();
        return false;
    }

    if (lImporter->IsFBX())
    {
        // Set the import states. By default, the import states are always set to 
        // true. The code below shows how to change these states.
        IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
        IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
        IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
        IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
        IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
        IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
        IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
    }

    // Import the scene
    lStatus = lImporter->Import(pScene);

    // Triangulate the mesh
    FbxGeometryConverter converter(m_sdkManager);
    converter.Triangulate(m_scene, true);
    
    // Set axis system
    FbxAxisSystem axisSystem = m_scene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem ourAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
    //FbxAxisSystem ourAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
    if (axisSystem != ourAxisSystem)
    {
        ourAxisSystem.ConvertScene(m_scene);
    }

    // Destroy the importer
    lImporter->Destroy();

    return lStatus;
}

// to get the root node
const FbxNode* FBXModel::GetRootNode()
{
    return m_scene->GetRootNode();
}

// to get the root node name
const char* FBXModel::GetRootNodeName()
{
    return GetRootNode()->GetName();
}

// to get a string from the node name and attribute type
FbxString FBXModel::GetNodeNameAndAttributeTypeName(const FbxNode* pNode)
{
    FbxString s = pNode->GetName();

    FbxNodeAttribute::EType lAttributeType;

    if (pNode->GetNodeAttribute() == nullptr)
    {
        s += " (No node attribute type)";
    }
    else
    {
        lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());

        switch (lAttributeType)
        {
        case FbxNodeAttribute::eMarker :            s += " (Marker)";               break;
        case FbxNodeAttribute::eSkeleton :          s += " (Skeleton)";             break;
        case FbxNodeAttribute::eMesh :              s += " (Mesh)";                 break;
        case FbxNodeAttribute::eCamera :            s += " (Camera)";               break;
        case FbxNodeAttribute::eLight :             s += " (Light)";                break;
        case FbxNodeAttribute::eBoundary :          s += " (Boundary)";             break;
        case FbxNodeAttribute::eOpticalMarker :     s += " (Optical marker)";       break;
        case FbxNodeAttribute::eOpticalReference :  s += " (Optical reference)";    break;
        case FbxNodeAttribute::eCameraSwitcher :    s += " (Camera switcher)";      break;
        case FbxNodeAttribute::eNull :              s += " (Null)";                 break;
        case FbxNodeAttribute::ePatch :             s += " (Patch)";                break;
        case FbxNodeAttribute::eNurbs :             s += " (NURB)";                 break;
        case FbxNodeAttribute::eNurbsSurface :      s += " (Nurbs surface)";        break;
        case FbxNodeAttribute::eNurbsCurve :        s += " (NURBS curve)";          break;
        case FbxNodeAttribute::eTrimNurbsSurface :  s += " (Trim nurbs surface)";   break;
        case FbxNodeAttribute::eUnknown :           s += " (Unidentified)";         break;
        }
    }
    return s;
}

// to get a string from the node default translation values
FbxString FBXModel::GetDefaultTranslationInfo(const FbxNode* pNode)
{
    FbxVector4 v4;
    v4 = ((FbxNode*)pNode)->LclTranslation.Get();

    return FbxString("Translation (X,Y,Z): ") + FbxString(v4[0]) + ", " + FbxString(v4[1]) + ", " + FbxString(v4[2]);
}

// to get a string from the node visibility value
FbxString FBXModel::GetNodeVisibility(const FbxNode* pNode)
{
    return FbxString("Visibility: ") + (pNode->GetVisibility() ? "Yes" : "No");
}

void FBXModel::LoadMeshes(FbxNode* pNode)
{
    FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();
    auto childCount = pNode->GetChildCount();

    if ((nullptr != pNodeAttribute) && (pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh))
    {
        LoadMesh(pNode);
    }

    for (int lChildIndex = 0; lChildIndex < childCount; ++lChildIndex)
    {
        LoadMeshes(pNode->GetChild(lChildIndex));
    }
}

void FBXModel::UploadMeshes()
//VOID FbxLoader::Mesh::UploadMesh(const std::vector<Vertex>& vertices, const std::vector<USHORT>& indices)
{
    ResourceUploadBatch resourceUpload(m_d3dDevice);
    resourceUpload.Begin();

    for (auto& mesh : m_meshes)
    //for (auto i = 0u; i < mMeshes.size(); i++)
    {
        mesh.numVertices = static_cast<uint32_t>(mesh.finalVertices.size());
        mesh.numIndices  = static_cast<uint32_t>(mesh.finalIndices32.size());
        //mMeshes[i].mNumIndices = UINT(mMeshes[i].mIndices.size());

        bool isIndex16 = mesh.numVertices <= UINT16_MAX;

        // Populate finalIndices16 vector if we can use _r16 index format.
        if (isIndex16)
        {
            std::copy(mesh.finalIndices32.begin(), mesh.finalIndices32.end(), std::back_inserter(mesh.finalIndices16));
            //for (auto& i : mesh.finalIndices32)
            //{
            //    mesh.finalIndices16.push_back(static_cast<uint16_t>(i));
            //}
        }

        mesh.vertexBufferSize = static_cast<uint32_t>(mesh.numVertices * sizeof(Vertex));
        mesh.skinnedVertexBufferSize = static_cast<uint32_t>(mesh.numVertices * sizeof(SkinnedVertex));
        mesh.indexBufferSize  = static_cast<uint32_t>(mesh.numIndices  * (isIndex16 ? sizeof(uint16_t) : sizeof(uint32_t)));
        //mMeshes[i].mVertexBufferSize = UINT(mMeshes[i].mVertices.size() * sizeof(Vertex));
        //mMeshes[i].mIndexBufferSize = UINT(mMeshes[i].mIndices.size() * sizeof(USHORT));

        ThrowIfFailed(CreateStaticBuffer(
            m_d3dDevice,
            resourceUpload,
            mesh.finalVertices,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            &mesh.vertexBuffer
        ));
        ThrowIfFailed(CreateStaticBuffer(
            m_d3dDevice,
            resourceUpload,
            mesh.finalSkinnedVertices,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            &mesh.skinnedVertexBuffer,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS // Buffer will be updated by a compute shader.
        ));
        // The data type of index buffer depends on vertex count.
        if (isIndex16)
        {
            ThrowIfFailed(CreateStaticBuffer(
                m_d3dDevice,
                resourceUpload,
                mesh.finalIndices16,
                D3D12_RESOURCE_STATE_INDEX_BUFFER,
                &mesh.indexBuffer
            ));
        }
        else
        {
            ThrowIfFailed(CreateStaticBuffer(
                m_d3dDevice,
                resourceUpload,
                mesh.finalIndices32,
                D3D12_RESOURCE_STATE_INDEX_BUFFER,
                &mesh.indexBuffer
            ));
        }

        mesh.vertexBufferView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
        mesh.vertexBufferView.SizeInBytes    = mesh.vertexBufferSize;
        mesh.vertexBufferView.StrideInBytes  = sizeof(Vertex);

        mesh.skinnedVertexBufferView.BufferLocation = mesh.skinnedVertexBuffer->GetGPUVirtualAddress();
        mesh.skinnedVertexBufferView.SizeInBytes    = mesh.skinnedVertexBufferSize;
        mesh.skinnedVertexBufferView.StrideInBytes  = sizeof(SkinnedVertex);

        mesh.indexBufferView.BufferLocation  = mesh.indexBuffer->GetGPUVirtualAddress();
        mesh.indexBufferView.SizeInBytes     = mesh.indexBufferSize;
        mesh.indexBufferView.Format          = isIndex16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

        //mesh.vertexBufferSharedResource = GraphicsMemory::Get().Allocate(mesh.vertexBufferSize);
        //mesh.indexBufferSharedResource = GraphicsMemory::Get().Allocate(mesh.indexBufferSize);
        //
        //memcpy(mesh.vertexBufferSharedResource.Memory(), mesh.finalVertices.data(), mesh.vertexBufferSize);
        //memcpy(mesh.indexBufferSharedResource.Memory(), mesh.finalIndices.data(),  mesh.indexBufferSize);
        ////memcpy(mMeshes[i].mSharedResourceVB.Memory(), mMeshes[i].mVertices.data(), mMeshes[i].mVertexBufferSize);
        ////memcpy(mMeshes[i].mSharedResourceIB.Memory(), mMeshes[i].mIndices.data(), mMeshes[i].mIndexBufferSize);
        //
        //mesh.heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        //mesh.vertexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mesh.vertexBufferSize);
        //mesh.indexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mesh.indexBufferSize);
        //
        //ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        //    &mesh.heapProperties, D3D12_HEAP_FLAG_NONE,
        //    &mesh.vertexBufferResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        //    IID_PPV_ARGS(&mesh.vertexBuffer)));
        //
        //ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        //    &mesh.heapProperties, D3D12_HEAP_FLAG_NONE,
        //    &mesh.indexBufferResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        //    IID_PPV_ARGS(&mesh.indexBuffer)));

        //mMeshes[i].mVertexBufferSize = UINT(mMeshes[i].mFinalVertices.size() * sizeof(Vertex));
        //mMeshes[i].mIndexBufferSize = UINT(mMeshes[i].mFinalIndices.size() * sizeof(USHORT));
        ////mMeshes[i].mVertexBufferSize = UINT(mMeshes[i].mVertices.size() * sizeof(Vertex));
        ////mMeshes[i].mIndexBufferSize = UINT(mMeshes[i].mIndices.size() * sizeof(USHORT));
        //
        //mMeshes[i].mNumIndices = UINT(mMeshes[i].mFinalIndices.size());
        ////mMeshes[i].mNumIndices = UINT(mMeshes[i].mIndices.size());
        //
        //mMeshes[i].mSharedResourceVB = GraphicsMemory::Get().Allocate(mMeshes[i].mVertexBufferSize);
        //mMeshes[i].mSharedResourceIB = GraphicsMemory::Get().Allocate(mMeshes[i].mIndexBufferSize);
        //
        //memcpy(mMeshes[i].mSharedResourceVB.Memory(), mMeshes[i].mFinalVertices.data(), mMeshes[i].mVertexBufferSize);
        //memcpy(mMeshes[i].mSharedResourceIB.Memory(), mMeshes[i].mFinalIndices.data(), mMeshes[i].mIndexBufferSize);
        ////memcpy(mMeshes[i].mSharedResourceVB.Memory(), mMeshes[i].mVertices.data(), mMeshes[i].mVertexBufferSize);
        ////memcpy(mMeshes[i].mSharedResourceIB.Memory(), mMeshes[i].mIndices.data(), mMeshes[i].mIndexBufferSize);
        //
        //mMeshes[i].mHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        //
        //mMeshes[i].mVBResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mMeshes[i].mVertexBufferSize);
        //mMeshes[i].mIBResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mMeshes[i].mIndexBufferSize);
    }

    auto uploadResourcesFinished = resourceUpload.End(m_commandQueue);
    uploadResourcesFinished.wait();
}

/*
void FbxLoader::CreateBufferResources(Mesh& mesh)
//void FbxLoader::CreateBufferResources(ID3D12Device* device, Mesh& mesh)
//VOID FbxLoader::CreateBufferResources(ID3D12Device* device, UINT index)
//VOID FbxLoader::Mesh::CreateBufferResources(ID3D12Device* device)
{
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &mesh.heapProperties, D3D12_HEAP_FLAG_NONE,
        &mesh.vertexBufferResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&mesh.vertexBuffer)));

    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &mesh.heapProperties, D3D12_HEAP_FLAG_NONE,
        &mesh.indexBufferResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&mesh.indexBuffer)));

    //ThrowIfFailed(device->CreateCommittedResource(
    //    &mMeshes[index].mHeapProperties, D3D12_HEAP_FLAG_NONE,
    //    &mMeshes[index].mVBResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
    //    IID_PPV_ARGS(mMeshes[index].mVertexBuffer.ReleaseAndGetAddressOf())));
    //
    //ThrowIfFailed(device->CreateCommittedResource(
    //    &mMeshes[index].mHeapProperties, D3D12_HEAP_FLAG_NONE,
    //    &mMeshes[index].mIBResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
    //    IID_PPV_ARGS(mMeshes[index].mIndexBuffer.ReleaseAndGetAddressOf())));
}

void FbxLoader::CreateBufferViews(Mesh& mesh)
//VOID FbxLoader::CreateBufferViews(UINT index)
//VOID FbxLoader::Mesh::CreateBufferViews()
{
    // set vertex & index buffer views only after buffer pointers are acquired and filled with data

    mesh.vertexBufferView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
    mesh.vertexBufferView.SizeInBytes = mesh.vertexBufferSize;
    mesh.vertexBufferView.StrideInBytes = sizeof(Vertex);

    mesh.indexBufferView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
    mesh.indexBufferView.SizeInBytes = mesh.indexBufferSize;
    mesh.indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    //mMeshes[index].mVertexBufferView.BufferLocation = mMeshes[index].mVertexBuffer->GetGPUVirtualAddress();
    //mMeshes[index].mVertexBufferView.SizeInBytes = mMeshes[index].mVertexBufferSize;
    //mMeshes[index].mVertexBufferView.StrideInBytes = UINT(sizeof(Vertex));
    //
    //mMeshes[index].mIndexBufferView.BufferLocation = mMeshes[index].mIndexBuffer->GetGPUVirtualAddress();
    //mMeshes[index].mIndexBufferView.SizeInBytes = mMeshes[index].mIndexBufferSize;
    //mMeshes[index].mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;

    // can also use array initialization method

    //vertexBufferView = { vertexBuffer->GetGPUVirtualAddress(), vertexBufferSize, UINT(sizeof(Vertex)) };
    //indexBufferView = { indexBuffer->GetGPUVirtualAddress(), indexBufferSize, DXGI_FORMAT_R16_UINT };
}
*/

// Debugger indicates that vertex extents are not being calculated correctly for bounding volumes.

void FBXModel::CreateMeshBoundingBoxes()
{
    for (auto& mesh : m_meshes)
    {
        BoundingBox::CreateFromPoints(
            mesh.boundingBox, mesh.finalVertices.size(), &mesh.finalVertices[0].pos, sizeof(Vertex));
    }
}

void FBXModel::CreateMeshBoundingSpheres()
{
    for (auto& mesh : m_meshes)
    {
        BoundingSphere::CreateFromPoints(
            mesh.boundingSphere, mesh.finalVertices.size(), &mesh.finalVertices[0].pos, sizeof(Vertex));
    }
}

/*
VOID FbxLoader::Mesh::ProcessSkeleton(FbxNode* pNode)
{
    FbxNodeAttribute::EType attType;
    FbxNode* pChildNode = nullptr;

    for (UINT i = 0u; i < UINT(pNode->GetChildCount()); i++)
    {
        pChildNode = pNode->GetChild(i);
        attType = pChildNode->GetNodeAttribute()->GetAttributeType();
        if (attType != FbxNodeAttribute::eSkeleton)
            continue;

        ProcessBoneHierarchy(pChildNode, 0, -1);
    }
}
*/

void FBXModel::LoadMesh(FbxNode* pNode)
{
    // Bake material and hook as user data.
    const auto materialCount = pNode->GetMaterialCount();
    FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();

    Mesh m;
    m_meshes.push_back(m);
    Mesh& mesh = m_meshes.back();

    // This paragraph of code isn't necessary, but it's nice to see.
    for (int lMaterialIndex = 0; lMaterialIndex < materialCount; ++lMaterialIndex)
    {
        FbxSurfaceMaterial* lMaterial = pNode->GetMaterial(lMaterialIndex);
        if (lMaterial && !lMaterial->GetUserDataPtr())
        {
            mesh.materialName = lMaterial->GetName();
        }
    }

    if (nullptr != pNodeAttribute)
    {
        if (pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            FbxMesh* pMesh = pNode->GetMesh();
            //FbxMesh* pMesh = (FbxMesh*)pChildNode->GetNodeAttribute();

            if (nullptr != pMesh)
            {
                auto boneCount = GetBoneCount(pMesh);
                mesh.meshName = pMesh->GetName(); // not necessary
                LoadControlPoints(pMesh, mesh);
                LoadNormalTexTangent(pMesh, mesh);

                if (boneCount)
                {
                    assert(IsMeshSkinned(pMesh));
                    // Ensure our bone matrix vector size is large enough.
                    assert(MaxBones >= boneCount);
                    LoadMeshBoneWeightsIndices(pMesh, mesh);
                    //LoadMeshBoneWeightsIndices(pNode, mesh);
                    //NormalizeBoneWeights(mesh);
                    CopyBoneWeightsToVertex(pMesh, mesh);
                    //CopyBoneWeightsToVertex(mesh);
                    DedupeVertices(mesh);
                    // CompressVertices(mesh);
                }
            }
        }
    }
}

void FBXModel::LoadNormalTexTangent(FbxMesh* pMesh, Mesh& mesh)
{
    auto polyCount = pMesh->GetPolygonCount(); // Number of polygons in mesh.
    uint32_t vCounter = 0; // Vertex counter.

    //std::vector<Vertex> vertices;
    //std::vector<USHORT> indices;

    for (int i = 0; i < polyCount; i++)
    {
        Vector3 normal[3];
        Vector3 tangent[3];
        Vector2 texCoord[3][1];
        //Vector2 tex[3][2];

        auto polyVerts = pMesh->GetPolygonSize(i);
        assert(polyVerts == 3);

        for (int j = 0; j < polyVerts; j++)
        {
            auto cpi = pMesh->GetPolygonVertex(i, j);
            CtrlPoint* currCP = mesh.controlPoints[cpi];
            ReadNormal(pMesh, cpi, vCounter, normal[j]);
            ReadTangent(pMesh, cpi, vCounter, tangent[j]);
            ReadTexCoord(pMesh, cpi, pMesh->GetTextureUVIndex(i, j), texCoord[j][0]);

            auto v  = Vertex(currCP->position, normal[j], texCoord[j][0], tangent[j]);
            auto sv = SkinnedVertex(currCP->position, normal[j], texCoord[j][0], tangent[j]);
            //Vertex v(currCP->position, normal[j], texCoord[j][0], tangent[j]);

            mesh.vertices.push_back(v);
            mesh.skinnedVertices.push_back(sv);
            mesh.indices.push_back(vCounter);
            ++vCounter;
        }
    }
}

void FBXModel::LoadControlPoints(FbxMesh* pMesh, Mesh& mesh)
{
    auto cpCount = pMesh->GetControlPointsCount();

    for (int i = 0; i < cpCount; i++)
    {
        CtrlPoint* currCP = new CtrlPoint();
        FbxVector4 currPos = pMesh->GetControlPointAt(i);
        
        currCP->position.x = static_cast<float>(currPos.mData[0]);
        currCP->position.y = static_cast<float>(currPos.mData[1]);
        currCP->position.z =-static_cast<float>(currPos.mData[2]); // Flip z for a RH world coord system.
        //currCP->mPosition.z = FLOAT(currPos.mData[2u]);

        mesh.controlPoints.insert(std::make_pair(i, currCP));
    }
}

void FBXModel::ReadNormal(FbxMesh* pMesh, int cpIndex, int vCounter, Vector3& n)
{
    if (pMesh->GetElementNormalCount() < 1)
        throw std::exception("Invalid Normal Number");

    FbxGeometryElementNormal* vNormal = pMesh->GetElementNormal(0);
    if (vNormal == nullptr) return;

    FbxVector4 fbxNrm;
    int index;
    switch (vNormal->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint :
        switch (vNormal->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect :
            fbxNrm = vNormal->GetDirectArray().GetAt(cpIndex);
            n.x = static_cast<float>(fbxNrm.mData[0]);
            n.y = static_cast<float>(fbxNrm.mData[1]);
            n.z =-static_cast<float>(fbxNrm.mData[2]); // Flip z for a RH world coord system.
            //n.x = FLOAT(vNormal->GetDirectArray().GetAt(cpIndex).mData[0u]);
            //n.y = FLOAT(vNormal->GetDirectArray().GetAt(cpIndex).mData[1u]);
            //n.z = FLOAT(vNormal->GetDirectArray().GetAt(cpIndex).mData[2u]);
            break;

        case FbxGeometryElement::eIndexToDirect :
            index = vNormal->GetIndexArray().GetAt(cpIndex);
            fbxNrm = vNormal->GetDirectArray().GetAt(index);
            n.x = static_cast<float>(fbxNrm.mData[0]);
            n.y = static_cast<float>(fbxNrm.mData[1]);
            n.z =-static_cast<float>(fbxNrm.mData[2]); // Flip z for a RH world coord system.
            //n.x = FLOAT(vNormal->GetDirectArray().GetAt(index).mData[0u]);
            //n.y = FLOAT(vNormal->GetDirectArray().GetAt(index).mData[1u]);
            //n.z = FLOAT(vNormal->GetDirectArray().GetAt(index).mData[2u]);
            break;

        default :
            throw std::exception("Invalid Reference");
        }
        break;

    case FbxGeometryElement::eByPolygonVertex :
        switch (vNormal->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect :
            fbxNrm = vNormal->GetDirectArray().GetAt(vCounter);
            n.x = static_cast<float>(fbxNrm.mData[0]);
            n.y = static_cast<float>(fbxNrm.mData[1]);
            n.z =-static_cast<float>(fbxNrm.mData[2]); // Flip z for a RH world coord system.
            //n.x = FLOAT(vNormal->GetDirectArray().GetAt(vCounter).mData[0u]);
            //n.y = FLOAT(vNormal->GetDirectArray().GetAt(vCounter).mData[1u]);
            //n.z = FLOAT(vNormal->GetDirectArray().GetAt(vCounter).mData[2u]);
            break;

        case FbxGeometryElement::eIndexToDirect :
            index = vNormal->GetIndexArray().GetAt(vCounter);
            fbxNrm = vNormal->GetDirectArray().GetAt(index);
            n.x = static_cast<float>(fbxNrm.mData[0]);
            n.y = static_cast<float>(fbxNrm.mData[1]);
            n.z =-static_cast<float>(fbxNrm.mData[2]); // Flip z for a RH world coord system.
            //n.x = FLOAT(vNormal->GetDirectArray().GetAt(index).mData[0u]);
            //n.y = FLOAT(vNormal->GetDirectArray().GetAt(index).mData[1u]);
            //n.z = FLOAT(vNormal->GetDirectArray().GetAt(index).mData[2u]);
            break;

        default :
            throw std::exception("Invalid Reference");
        }
        break;
    }
}

void FBXModel::ReadTexCoord(FbxMesh* pMesh, int cpIndex, int uvIndex, Vector2& t)
{
    FbxLayerElementUV* vTexC = pMesh->GetLayer(0)->GetUVs();
    if (vTexC == nullptr) return;

    FbxVector2 fbxTex;
    int index;
    switch (vTexC->GetMappingMode())
    {
    case FbxLayerElementUV::eByControlPoint :
        switch (vTexC->GetReferenceMode())
        {
        case FbxLayerElementUV::eDirect :
            fbxTex = vTexC->GetDirectArray().GetAt(cpIndex);
            t.x = static_cast<float>(fbxTex.mData[0]);
            t.y = 1.f - static_cast<float>(fbxTex.mData[1]);
            break;

        case FbxLayerElementUV::eIndexToDirect :
            index = vTexC->GetIndexArray().GetAt(cpIndex);
            fbxTex = vTexC->GetDirectArray().GetAt(index);
            t.x = static_cast<float>(fbxTex.mData[0]);
            t.y = 1.f - static_cast<float>(fbxTex.mData[1]);
            break;
        
        default :
            throw std::exception("Invalid Reference");
        }
        break;

    case FbxLayerElementUV::eByPolygonVertex :
        switch (vTexC->GetReferenceMode())
        {
        // Always enters this part for the example model
        case FbxLayerElementUV::eDirect :
        case FbxLayerElementUV::eIndexToDirect :
            fbxTex = vTexC->GetDirectArray().GetAt(uvIndex);
            t.x = static_cast<float>(fbxTex.mData[0]);
            t.y = 1.f - static_cast<float>(fbxTex.mData[1]);
            break;

        default:
            throw std::exception("Invalid Reference");
        }
        break;
    }
}

void FBXModel::ReadTangent(FbxMesh* pMesh, int cpIndex, int vCounter, Vector3& u)
{
    if (pMesh->GetElementTangentCount() < 1)
        throw std::exception("Invalid Tangent Number");

    FbxGeometryElementTangent* vTangent = pMesh->GetElementTangent(0);
    if (vTangent == nullptr) return;

    FbxVector4 fbxTan;
    int index;
    switch (vTangent->GetMappingMode())
    {
    case FbxGeometryElement::eByControlPoint:
        switch (vTangent->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
            fbxTan = vTangent->GetDirectArray().GetAt(cpIndex);
            u.x = static_cast<float>(fbxTan.mData[0]);
            u.y = static_cast<float>(fbxTan.mData[1]);
            u.z =-static_cast<float>(fbxTan.mData[2]); // Flip z for a RH world coord system.
            break;

        case FbxGeometryElement::eIndexToDirect:
            index = vTangent->GetIndexArray().GetAt(cpIndex);
            fbxTan = vTangent->GetDirectArray().GetAt(index);
            u.x = static_cast<float>(fbxTan.mData[0]);
            u.y = static_cast<float>(fbxTan.mData[1]);
            u.z =-static_cast<float>(fbxTan.mData[2]); // Flip z for a RH world coord system.
            break;

        default:
            throw std::exception("Invalid Reference");
        }
        break;

    case FbxGeometryElement::eByPolygonVertex:
        switch (vTangent->GetReferenceMode())
        {
        case FbxGeometryElement::eDirect:
            fbxTan = vTangent->GetDirectArray().GetAt(vCounter);
            u.x = static_cast<float>(fbxTan.mData[0]);
            u.y = static_cast<float>(fbxTan.mData[1]);
            u.z =-static_cast<float>(fbxTan.mData[2]); // Flip z for a RH world coord system.
            break;

        case FbxGeometryElement::eIndexToDirect:
            index = vTangent->GetIndexArray().GetAt(vCounter);
            fbxTan = vTangent->GetDirectArray().GetAt(index);
            u.x = static_cast<float>(fbxTan.mData[0]);
            u.y = static_cast<float>(fbxTan.mData[1]);
            u.z =-static_cast<float>(fbxTan.mData[2]); // Flip z for a RH world coord system.
            break;

        default:
            throw std::exception("Invalid Reference");
        }
        break;
    }
}

/*
VOID FbxLoader::Mesh::ProcessBoneHierarchy(FbxNode* pNode, UINT index, UINT parentIndex)
{
    FbxNodeAttribute::EType attType = pNode->GetNodeAttribute()->GetAttributeType();

    if (attType == FbxNodeAttribute::eSkeleton)
    {
        Joint currJoint;
        currJoint.mParentIndex = parentIndex;
        currJoint.mName = pNode->GetName();
        mSkeleton->mJoints.push_back(currJoint);
    }

    for (UINT i = 0u; i < pNode->GetChildCount(); i++)
    {
        ProcessBoneHierarchy(pNode->GetChild(i), mSkeleton->mJoints.size(), index);
    }
}

VOID FbxLoader::Mesh::ProcessJointAnimation(FbxNode* pNode)
{
    FbxMesh* currMesh = pNode->GetMesh();
    UINT numDeformers = currMesh->GetDeformerCount();

    // This geometry transform is something I cannot understand 
    // I think it is from MotionBuilder 
    // If you are using Maya for your models, 99% this is just an 
    // identity matrix 
    // But I am taking it into account anyways...... 

    FbxAMatrix geometryTransform;
    geometryTransform = GetGeometryTransformation(pNode); // function must be declared static

    // A deformer is a FBX thing, which contains some clusters 
    // A cluster contains a link, which is basically a joint 
    // Normally, there is only one deformer in a mesh 
    for (UINT deformerIndex = 0u; deformerIndex < numDeformers; deformerIndex++)
    {
        // There are many types of deformers in Maya, 
        // We are using only skins, so we see if this is a skin 
        FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(currMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));

        if (!currSkin) continue;

        UINT numClusters = currSkin->GetClusterCount();
        for (UINT clusterIndex = 0u; clusterIndex < numClusters; clusterIndex++)
        {
            FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
            std::string currJointName = currCluster->GetLink()->GetName();
            UINT currJointIndex = FindJointIndexUsingName(currJointName);

            FbxAMatrix transformMatrix;
            FbxAMatrix transformLinkMatrix;
            FbxAMatrix globalBindposeInverseMatrix;
            currCluster->GetTransformMatrix(transformMatrix);

            // The transformation of the mesh at binding time 
            currCluster->GetTransformLinkMatrix(transformLinkMatrix);

            // The transformation of the cluster(joint) at binding time from joint space to world space 
            globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

            // Update the information in mSkeleton 
            mSkeleton->mJoints[currJointIndex].mGlobalBindposeInverse = globalBindposeInverseMatrix;
            mSkeleton->mJoints[currJointIndex].mNode = currCluster->GetLink();

            // Associate each joint with the control points it affects 
            UINT numIndices = currCluster->GetControlPointIndicesCount();
            for (UINT i = 0u; i < numIndices; i++)
            {
                BlendingIndexWeightPair currBlendingIndexWeightPair;
                currBlendingIndexWeightPair.mBlendingIndex = currJointIndex;

                currBlendingIndexWeightPair.mBlendingWeight = FLOAT(currCluster->GetControlPointWeights()[i]);
                mControlPoints[currCluster->GetControlPointIndices()[i]]->
                    mBlendingInfo.push_back(currBlendingIndexWeightPair);
            }

            // Get animation information 
            // Now only supports one take 
            FbxAnimStack* currAnimStack = mScene->GetSrcObject<FbxAnimStack>(0u);
            FbxString animStackName = currAnimStack->GetName();
            mAnimationName = animStackName.Buffer();
            FbxTakeInfo* takeInfo = mScene->GetTakeInfo(animStackName);
            FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
            FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
            mAnimationLength = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1u;
            Keyframe** currAnim = &mSkeleton->mJoints[currJointIndex].mAnimation;

            for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); i++)
            {
                FbxTime currTime;
                currTime.SetFrame(i, FbxTime::eFrames24);
                *currAnim = new Keyframe();
                (*currAnim)->mFrameNum = i;
                FbxAMatrix currentTransformOffset = pNode->EvaluateGlobalTransform(currTime) * geometryTransform;
                (*currAnim)->mGlobalTransform = currentTransformOffset.Inverse()
                    * currCluster->GetLink()->EvaluateGlobalTransform(currTime);
                currAnim = &((*currAnim)->mNext);
            }
        }
    }

    // Some of the control points only have less than 4 joints 
    // affecting them. 
    // For a normal renderer, there are usually 4 joints 
    // I am adding more dummy joints if there isn't enough 
    BlendingIndexWeightPair currBlendingIndexWeightPair;
    currBlendingIndexWeightPair.mBlendingIndex = 0u;
    currBlendingIndexWeightPair.mBlendingWeight = 0.f;
    for (auto itr = mControlPoints.begin(); itr != mControlPoints.end(); ++itr)
    {
        for (UINT i = itr->second->mBlendingInfo.size(); i <= 4u; ++i)
        {
            itr->second->mBlendingInfo.push_back(currBlendingIndexWeightPair);
        }
    }
}
*/

int FBXModel::BoneNameToIndex(const std::string& boneName)
{
    int index = 0;
    for (auto const& i : m_boneVector)
    {
        if (i.name == boneName)
        {
            return index;
        }

        ++index;
    }
    assert(false);
    return -1;
}

void FBXModel::GetGeometryTransformMatrix(FbxNode* pNode, FbxAMatrix& offsetMatrix)
//VOID FbxLoader::GetGeometryTransformMatrix(FbxNode* pNode, Matrix& offsetMatrix)
{
    const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
    const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

    offsetMatrix = FbxAMatrix(lT, lR, lS);
    //FbxAMatrix fbxMatrix = FbxAMatrix(lT, lR, lS);
    //FbxToMatrix(fbxMatrix, offsetMatrix);
}

void FBXModel::LoadBone(FbxNode* pNode, int parentBoneIndex)
{
    Bone dummy;
    m_boneVector.push_back(dummy);
    Bone& back = m_boneVector.back();

    back.boneNodePtr = pNode;
    back.fbxSkeletonPtr = (FbxSkeleton*)pNode->GetNodeAttribute();
    back.fbxClusterPtr = nullptr; // fix this up later
    back.name = back.fbxSkeletonPtr->GetName();
    back.parentIndex = parentBoneIndex;

    // We aren't ready to load these, so set them to identity.
    //D3DXMatrixIdentity(&back.offset);
    //D3DXMatrixIdentity(&back.boneMatrice);
    //D3DXMatrixIdentity(&back.combinedTransform);

    // Get our local transform at time 0.
    GetNodeLocalTransform(pNode, back.nodeLocalTransform);

    if (parentBoneIndex != -1)
    {
        m_boneVector[parentBoneIndex].childIndexes.push_back(static_cast<int>(m_boneVector.size() - 1));
    }
}

void FBXModel::LoadBones(FbxNode* pNode, int parentBoneIndex)
{
    FbxNodeAttribute* pNodeAttribute = pNode->GetNodeAttribute();
    auto childCount = pNode->GetChildCount();

    if ((nullptr != pNodeAttribute)
        && (pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton))
    {
        LoadBone(pNode, parentBoneIndex);
        parentBoneIndex = static_cast<int>(m_boneVector.size() - 1);
    }

    for (int lChildIndex = 0; lChildIndex < childCount; ++lChildIndex)
    {
        LoadBones(pNode->GetChild(lChildIndex), parentBoneIndex);
    }
}

void FBXModel::GetNodeLocalTransform(FbxNode* pNode, Matrix& matrix)
{
    FbxAMatrix fbxMatrix = m_scene->GetAnimationEvaluator()->GetNodeLocalTransform(pNode);
    FbxToMatrix(fbxMatrix, matrix);
}

void FBXModel::GetNodeLocalTransform(FbxNode* pNode, const FbxTime& fbxTime, Matrix& matrix)
{
    FbxAMatrix fbxMatrix = m_scene->GetAnimationEvaluator()->GetNodeLocalTransform(pNode, fbxTime);
    FbxToMatrix(fbxMatrix, matrix);
}

void FBXModel::FbxToMatrix(const FbxAMatrix& fbxMatrix, Matrix& matrix)
{
    for (int i = 0; i < 4; ++i)
    {
        matrix(i,0) = static_cast<float>(fbxMatrix.Get(i,0));
        matrix(i,1) = static_cast<float>(fbxMatrix.Get(i,1));
        matrix(i,2) = static_cast<float>(fbxMatrix.Get(i,2));
        matrix(i,3) = static_cast<float>(fbxMatrix.Get(i,3));
    }
}

int FBXModel::GetBoneCount(FbxMesh* pMesh)
{
    if (pMesh->GetDeformerCount(FbxDeformer::eSkin) == 0)
    {
        return 0;
    }

    return ((FbxSkin*)(pMesh->GetDeformer(0, FbxDeformer::eSkin)))->GetClusterCount();
}

bool FBXModel::IsMeshSkinned(FbxMesh* pMesh)
{
    auto boneCount = GetBoneCount(pMesh);
    return boneCount > 0;
}

void FBXModel::LoadMeshBoneWeightsIndices(FbxMesh* pMesh, Mesh& mesh)
//VOID FbxLoader::LoadMeshBoneWeightsIndices(FbxNode* pNode, Mesh& mesh)
{
    //FbxMesh* pMesh = pNode->GetMesh();
    //FbxAMatrix geometryTransform;
    //Matrix geometryTransform;
    //tControlPointRemap controlPointRemap;

    // maps control points to vertex indexes
    //LoadControlPointRemap(pMesh, controlPointRemap);

    // takes into account an offsetted model.
    //GetGeometryTransformMatrix(pNode, geometryTransform);

    // A deformer is a FBX thing, which contains some clusters
    // A cluster contains a link, which is basically a joint
    // Normally, there is only one deformer in a mesh
    // There are many types of deformers in Maya,
    // We are using only skins, so we see if this is a skin
    FbxSkin* currSkin = (FbxSkin*)(pMesh->GetDeformer(0, FbxDeformer::eSkin));

    auto numberOfClusters = currSkin->GetClusterCount();
    for (int clusterIndex = 0; clusterIndex < numberOfClusters; ++clusterIndex)
    {
        FbxCluster* clusterPtr = currSkin->GetCluster(clusterIndex);
        auto numOfIndices = clusterPtr->GetControlPointIndicesCount();
        auto weightPtr = clusterPtr->GetControlPointWeights();
        auto indicePtr = clusterPtr->GetControlPointIndices();
        std::string currJointName = clusterPtr->GetLink()->GetName();
        std::string secondName = clusterPtr->GetName();
        auto boneIndex = BoneNameToIndex(currJointName);
        FbxAMatrix transformMatrix;
        FbxAMatrix transformLinkMatrix;
        FbxAMatrix globalBindposeInverseMatrix;

        // Now that we have the clusterPtr, let's calculate the inverse bind pose matrix.

        // The transformation of the cluster(joint) at binding time from joint space to world space
        clusterPtr->GetTransformLinkMatrix(transformLinkMatrix);
        clusterPtr->GetTransformMatrix(transformMatrix); // The transformation of the mesh at binding time
        // find out if we need geometryTransform
        globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix;// *geometryTransform;
        FbxToMatrix(globalBindposeInverseMatrix, m_boneVector[boneIndex].offset);

        // Update the information in mSkeleton 
        m_boneVector[boneIndex].fbxClusterPtr = clusterPtr;

        if (nullptr == clusterPtr->GetLink())
        {
            continue;
        }

        // Associate each joint with the control points it affects
        for (int i = 0; i < numOfIndices; ++i)
        {
            BlendingIndexWeightPair currBlendingIndexWeightPair;
            currBlendingIndexWeightPair.blendingIndex = boneIndex;

            currBlendingIndexWeightPair.blendingWeight = static_cast<float>(clusterPtr->GetControlPointWeights()[i]);
            mesh.controlPoints[clusterPtr->GetControlPointIndices()[i]]->blendingInfo.push_back(currBlendingIndexWeightPair);
            
            //FLOAT weight = FLOAT(weightPtr[i]);
            //
            //if (weight == 0.f)
            //{
            //    continue;
            //}
            //
            //// all the points the control word is mapped to
            //tControlPointIndexes& controlPointIndexes = controlPointRemap[indicePtr[i]];
            //
            //for (auto const& i : controlPointIndexes)
            //{
            //    // Change the index vector offset, to a vertex offset.
            //    UINT vertexIndex = mesh.mIndexVector[i];
            //
            //    AddBoneInfluence(mesh.mVerticeVector, vertexIndex, boneIndex, weight);
            //}
        }
    }

    // Some of the control points only have less than 4 joints 
    // affecting them. 
    // For a normal renderer, there are usually 4 joints 
    // I am adding more dummy joints if there isn't enough 
    BlendingIndexWeightPair currBlendingIndexWeightPair;
    currBlendingIndexWeightPair.blendingIndex = 0;
    currBlendingIndexWeightPair.blendingWeight = 0;
    for (auto itr = mesh.controlPoints.begin(); itr != mesh.controlPoints.end(); ++itr)
    {
        for (size_t i = itr->second->blendingInfo.size(); i <= 4; ++i)
        {
            itr->second->blendingInfo.push_back(currBlendingIndexWeightPair);
        }
    }
}

void FBXModel::LoadControlPointRemap(FbxMesh* pMesh,tControlPointRemap& controlPointRemap)
{
    const auto lPolygonCount = pMesh->GetPolygonCount();
    controlPointRemap.resize(lPolygonCount);

    for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++)
    {
        auto lPolygonSize = pMesh->GetPolygonSize(lPolygonIndex);

        for (int lVertexIndex = 0; lVertexIndex < lPolygonSize; lVertexIndex++)
        {
            auto lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVertexIndex);
            controlPointRemap[lControlPointIndex].push_back(lPolygonIndex * 3 + lVertexIndex);
        }
    }
}

void FBXModel::AddBoneInfluence(std::vector<IndexWeightPair>& skinnedVerticeVector,
    int vertexIndex, int boneIndex, float boneWeight)
//VOID FbxLoader::AddBoneInfluence(tSkinnedVerticeVector& skinnedVerticeVector,
// UINT vertexIndex, UINT boneIndex, FLOAT boneWeight)
{
    uint32_t integerWeight = clamp(static_cast<uint32_t>(boneWeight * 255.f + 0.5f), 0u, 255u);
    //unsigned long integerWeight = clamp(static_cast<unsigned long>(boneWeight * 255.0 + 0.5)
    //, unsigned long(0), unsigned long(255));

    if (0 == integerWeight)
    {
        return;
    }

    PackedInt packedIndices = {};
    PackedInt packedWeights = {};
    uint32_t smallestWeightIndex = 0xFFFFFFFF;
    uint32_t smallestWeight = 0xFFFFFFFF;

    packedIndices.number = skinnedVerticeVector[vertexIndex].boneIndices;
    packedWeights.number = skinnedVerticeVector[vertexIndex].boneWeights;

    for (int i = 0; i < 4; ++i)
    {
        auto packedWeight = packedWeights.bytes[i];

        if (packedWeight == 0)
        {
            // Easy case! We found an empty slot.
            smallestWeightIndex = i;
            smallestWeight = 0;
            // We found an empty slot, but we keep looking in case of a duplicate.
        }
        else if ((packedWeight < boneWeight) && (packedWeight < smallestWeight))
        {
            // This slot is taken, but remember it incase we are full, and it's smaller
            // than the new weight.
            smallestWeightIndex = i;
            smallestWeight = packedWeight;
        }

        if (packedIndices.bytes[i] == boneIndex)
        { // This is a duplicate!?!
            return;
        }
    }

    if (smallestWeightIndex == 0xFFFFFFFF)
    {
        // We had more than four weights, and this weight is the smallest.
        return;
    }

    packedIndices.bytes[smallestWeightIndex] = static_cast<uint8_t>(boneIndex);
    packedWeights.bytes[smallestWeightIndex] = static_cast<uint8_t>(integerWeight);
    skinnedVerticeVector[vertexIndex].boneIndices = packedIndices.number;
    skinnedVerticeVector[vertexIndex].boneWeights = packedWeights.number;
}

void FBXModel::NormalizeBoneWeights(Mesh& mesh)
{
    for (auto& vertice : mesh.verticeVector)
    {
        PackedInt packedWeights = {};
        packedWeights.number = vertice.boneWeights;

        {
            auto totalWeight =
                  packedWeights.bytes[0]
                + packedWeights.bytes[1]
                + packedWeights.bytes[2]
                + packedWeights.bytes[3];

            assert(totalWeight != 0);

            if (totalWeight >= 254 && totalWeight <= 256)
            { // no need to normalize
                continue;
            }
        }

        float weights[4] = {};
        float calculation[4] = {};

        weights[0] = static_cast<float>(packedWeights.bytes[0]);
        weights[1] = static_cast<float>(packedWeights.bytes[1]);
        weights[2] = static_cast<float>(packedWeights.bytes[2]);
        weights[3] = static_cast<float>(packedWeights.bytes[3]);

        auto totalWeight = weights[0u] + weights[1u] + weights[2u] + weights[3u];

        calculation[0] = 255.f * weights[0] / totalWeight + 0.5f;
        calculation[1] = 255.f * weights[1] / totalWeight + 0.5f;
        calculation[2] = 255.f * weights[2] / totalWeight + 0.5f;
        calculation[3] = 255.f * weights[3] / totalWeight + 0.5f;

        packedWeights.bytes[0] = static_cast<uint8_t>(calculation[0]);
        packedWeights.bytes[1] = static_cast<uint8_t>(calculation[1]);
        packedWeights.bytes[2] = static_cast<uint8_t>(calculation[2]);
        packedWeights.bytes[3] = static_cast<uint8_t>(calculation[3]);

        vertice.boneWeights = packedWeights.number;
    }
}

void FBXModel::CopyBoneWeightsToVertex(Mesh& mesh)
{
    for (size_t i = 0; i < mesh.vertices.size(); i++)
    {
        PackedInt packedIndices = {};
        PackedInt packedWeights = {};

        packedIndices.number = mesh.verticeVector[i].boneIndices;
        packedWeights.number = mesh.verticeVector[i].boneWeights;

        float* boneWeight = &mesh.vertices[i].boneWeights.x;
        for (int j = 0; j < 4; j++)
        {
            mesh.vertices[i].boneIndices[j] = packedIndices.bytes[j];
            *boneWeight = static_cast<float>(packedWeights.bytes[j] / 255.f);
            //mesh.vertices[i].boneWeights[j] = static_cast<float>(packedWeights.bytes[j] / 255.f);
            boneWeight++; // Increment reference to the next vector component.
        }
    }

    // unallocate control point memory

    for (auto i = mesh.controlPoints.begin(); i != mesh.controlPoints.end(); i++)
        delete i->second;
    mesh.controlPoints.clear();
}

void FBXModel::CopyBoneWeightsToVertex(FbxMesh* pMesh, Mesh& mesh)
{
    auto polyCount = pMesh->GetPolygonCount(); // number of polygons in mesh
    int vCounter = 0; // vertex counter

    for (int i = 0; i < polyCount; i++)
    {
        auto polyVerts = pMesh->GetPolygonSize(i);
        assert(polyVerts == 3);

        for (int j = 0; j < polyVerts; j++)
        {
            auto cpi = pMesh->GetPolygonVertex(i, j);
            CtrlPoint* currCP = mesh.controlPoints[cpi];

            // Copy the blending info from each control point 
            float* boneWeight = &mesh.vertices[vCounter].boneWeights.x;
            for (int k = 0; k < 4; k++)
            {
                //BlendingIndexWeightPair currBlendingInfo;
                //currBlendingInfo.mBlendingIndex = currCP->mBlendingInfo[k].mBlendingIndex;
                //currBlendingInfo.mBlendingWeight = currCP->mBlendingInfo[k].mBlendingWeight;

                mesh.vertices[vCounter].boneIndices[k] = currCP->blendingInfo[k].blendingIndex;
                *boneWeight = currCP->blendingInfo[k].blendingWeight;
                //mesh.vertices[vCounter].boneWeights[k] = currCP->blendingInfo[k].blendingWeight;
                boneWeight++; // Increment reference to the next vector component.
            }
            ++vCounter;
        }
    }
    // unallocate control point memory

    for (auto i = mesh.controlPoints.begin(); i != mesh.controlPoints.end(); i++)
        delete i->second;
    mesh.controlPoints.clear();
}

void FBXModel::AdvanceTime(float time)
{
    FbxTime fbxFrameTime;
    FbxLongLong localAnimTime = static_cast<size_t>(1000.f * time) % m_initialAnimDuration_ms;
    //unsigned long long localAnimationTime = GetTickCount64() % m_initialAnimationDurationInMs;
    
    fbxFrameTime.SetMilliSeconds(localAnimTime);
    
    BuildMatrices(fbxFrameTime);
}

void FBXModel::Draw(ID3D12GraphicsCommandList* commandList)
{
    for (const auto& mesh : m_meshes)
    //for (auto i = 0u; i < mMeshes.size(); i++)
    //for (UINT i = 0u; i < mFbx.mMeshes.size(); i++)
    //for (UINT i = 0; i < m_assimpModel.GetMeshSize(); i++)
    {
        commandList->IASetVertexBuffers(0, 1, &mesh.vertexBufferView);
        commandList->IASetIndexBuffer(&mesh.indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(mesh.numIndices, 1, 0, 0, 0);

        //commandList->IASetVertexBuffers(0u, 1u, &mMeshes[i].mVertexBufferView);
        //commandList->IASetIndexBuffer(&mMeshes[i].mIndexBufferView);
        //commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //commandList->DrawIndexedInstanced(mMeshes[i].mNumIndices, 1u, 0u, 0u, 0u);
    }
}

void FBXModel::DrawSkinned(ID3D12GraphicsCommandList* commandList)
{
    for (const auto& mesh : m_meshes)
    {
        commandList->IASetVertexBuffers(0, 1, &mesh.skinnedVertexBufferView);
        commandList->IASetIndexBuffer(&mesh.indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawIndexedInstanced(mesh.numIndices, 1, 0, 0, 0);
    }
}

size_t FBXModel::GetAnimationDuration()
{
    FbxArray<FbxString*> animStackNameArray;
    FbxAnimStack* fbxAnimStackPtr = m_scene->GetSrcObject<FbxAnimStack>(0);
    assert(nullptr != fbxAnimStackPtr);

    m_scene->FillAnimStackNameArray(animStackNameArray);
    auto animStackNameArraySize = animStackNameArray.GetCount();
    assert(animStackNameArraySize >= 1);

    FbxTakeInfo* currentTakeInfoPtr = m_scene->GetTakeInfo(*animStackNameArray[0]);
    assert(nullptr != currentTakeInfoPtr);

    wchar_t buff[256] = {};
    swprintf_s(buff,
        L"currentTakeInfoPtr->mLocalTimeSpan.GetStop().GetMilliSeconds() = %llu.\n\
        currentTakeInfoPtr->mLocalTimeSpan.GetStart().GetMilliSeconds() = %llu.\n",
        currentTakeInfoPtr->mLocalTimeSpan.GetStop().GetMilliSeconds(),
        currentTakeInfoPtr->mLocalTimeSpan.GetStart().GetMilliSeconds());
    OutputDebugStringW(buff);
    size_t animTime =
        currentTakeInfoPtr->mLocalTimeSpan.GetStop().GetMilliSeconds()
        - currentTakeInfoPtr->mLocalTimeSpan.GetStart().GetMilliSeconds();

    return animTime;
}

void FBXModel::BuildMatrices(const FbxTime& frameTime)
{
    if (m_boneVector.empty())
    {
        return;
    }

    // Here's most of the matrix bone operations from start to finish.
    // The exception is the offset matrix calculation that happens once
    // when the bone is created.

    // Set the matrices that change by time and animation keys.
    LoadNodeLocalTransformMatrices(frameTime);

    // Propagate the local transform matrices from parent to child.
    CalculateCombinedTransforms();

    // This is the final pass that prepends the offset matrix.
    CalculatePaletteMatrices();
}

void FBXModel::CalculateCombinedTransforms()
{
    if (m_boneVector.empty())
    {
        return;
    }

    if (m_boneVector.size() == 1)
    {
        return;
    }

    for (auto& bone : m_boneVector)
    {
        if (bone.parentIndex != -1)
        {
            bone.combinedTransform = bone.nodeLocalTransform * m_boneVector[bone.parentIndex].combinedTransform;
        }
        else
        {
            bone.combinedTransform = bone.nodeLocalTransform;
        }
    }
}

void FBXModel::LoadNodeLocalTransformMatrices(const FbxTime& frameTime)
{
    for (size_t i = 0; i < m_boneVector.size(); ++i)
    {
        GetNodeLocalTransform(m_boneVector[i].boneNodePtr, frameTime, m_boneVector[i].nodeLocalTransform);
    }
}

void FBXModel::CalculatePaletteMatrices()
{
    m_bonePalette.clear();

    for (auto& bone : m_boneVector)
    {
        bone.boneMatrice = bone.offset * bone.combinedTransform;
        m_bonePalette.push_back(bone.boneMatrice);
    }

    // Added XMFLOAT3X4 packing code here to save the game from doing it when skinned models are rendered.

    int i = 0;
    for (const auto& bone : m_bonePalette)
    //for (auto i = 0u; i < mBonePalette.size(); i++)
    {
        // We do not transpose bone matrices for HLSL because the original FbxMatrix is in column-vector math format.
        // XMFLOAT3X4 matrices are also column-major, so can be also be loaded without transpose.

        XMStoreFloat3x4(&m_bonePalette3X4[i++], bone);
        //XMStoreFloat3x4(&mBonePalette3X4[i], mBonePalette[i]);
    }
}

int FBXModel::FindVertex(const std::vector<Vertex>& finalVertices, const Vertex& v)
//uint16_t FbxLoader::FindVertex(const std::vector<Vertex>& finalVertices, const Vertex& v)
{
    // I'm doing an old fashion sequential search. I figure it's not too awful with processor memory caches.
    // Also, the code is easier to read.
    for (uint32_t i = 0; i < static_cast<uint32_t>(finalVertices.size()); ++i)
    {
        if (v.pos == finalVertices[i].pos
            && v.normal == finalVertices[i].normal
            && v.texC == finalVertices[i].texC
            && v.tangent == finalVertices[i].tangent
            )
        {
            return i;
        }
    }
    return UINT32_MAX;
    //return 0xFFFF;
}

void FBXModel::DedupeVertices(Mesh& mesh)
{
    //tSkinnedVerticeVector newVertices;
    uint32_t findIndex = 0;
    //uint16_t findIndex = 0;

    for (auto& i : mesh.indices)
    {
        Vertex testVertex = mesh.vertices[i];
        SkinnedVertex tsv = mesh.skinnedVertices[i]; // The corresponding skinned vertex.

        findIndex = FindVertex(mesh.finalVertices, testVertex);

        if (findIndex == UINT32_MAX) // Not found.
        //if (findIndex == 0xFFFF) // Not found.
        {
            i = static_cast<uint32_t>(mesh.finalVertices.size());
            //i = static_cast<uint16_t>(mesh.finalVertices.size());
            mesh.finalVertices.push_back(testVertex);
            mesh.finalSkinnedVertices.push_back(tsv);
        }
        else
        {
            i = findIndex;
        }
        mesh.finalIndices32.push_back(i);
        //mesh.finalIndices.push_back(i);
    }
    //modelRec.verticeVector.swap(newVertices);
}
