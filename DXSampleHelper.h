//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "pch.h"

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
//using Microsoft::WRL::ComPtr;

namespace DX
{
    class HrException : public std::runtime_error
    {
        inline std::string HrToString(HRESULT hr)
        {
            char s_str[64] = {};
            sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
            return std::string(s_str);
        }
    public:
        HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
        HRESULT Error() const { return m_hr; }
    private:
        const HRESULT m_hr;
    };

#define SAFE_RELEASE(p) if (p) (p)->Release()

    //inline void ThrowIfFailed(HRESULT hr)
    //{
    //    if (FAILED(hr))
    //    {
    //        throw HrException(hr);
    //    }
    //}

    inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
    {
        if (FAILED(hr))
        {
            OutputDebugString(msg);
            throw HrException(hr);
        }
    }

    inline void ThrowIfFalse(bool value)
    {
        ThrowIfFailed(value ? S_OK : E_FAIL);
    }

    inline void ThrowIfFalse(bool value, const wchar_t* msg)
    {
        ThrowIfFailed(value ? S_OK : E_FAIL, msg);
    }
}

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    WCHAR* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size)
{
    using namespace Microsoft::WRL;

    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
    if (file.Get() == INVALID_HANDLE_VALUE)
    {
        throw std::exception();
    }

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        throw std::exception();
    }

    if (fileInfo.EndOfFile.HighPart != 0)
    {
        throw std::exception();
    }

    *data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
    *size = fileInfo.EndOfFile.LowPart;

    if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        throw std::exception();
    }

    return S_OK;
}

// Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
    WCHAR fullName[50];
    if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
    {
        pObject->SetName(fullName);
    }
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

inline UINT Align(UINT size, UINT alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
    // Constant buffer size is required to be aligned.
    return Align(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr;

    Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    return byteCode;
}
#endif

// Resets all elements in a ComPtr array.
template<class T>
void ResetComPtrArray(T* comPtrArray)
{
    for (auto &i : *comPtrArray)
    {
        i.Reset();
    }
}

// Resets all elements in a unique_ptr array.
template<class T>
void ResetUniquePtrArray(T* uniquePtrArray)
{
    for (auto &i : *uniquePtrArray)
    {
        i.reset();
    }
}

class GpuUploadBuffer
{
public:
    const auto GetResource() { return m_resource.Get(); }
    //Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() { return m_resource; }

protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    GpuUploadBuffer() {}
    ~GpuUploadBuffer()
    {
        if (m_resource.Get())
        {
            m_resource->Unmap(0, nullptr);
        }
    }

    void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
    {
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_resource)));
        m_resource->SetName(resourceName);
    }
    // Added this method to create resources compatible with enhanced barriers.
    void AllocateWithLayout(ID3D12Device10* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
    {
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        auto bufferDesc = CD3DX12_RESOURCE_DESC1::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource3(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_BARRIER_LAYOUT_UNDEFINED,
            nullptr, nullptr, 0, nullptr,
            IID_PPV_ARGS(&m_resource)));
        m_resource->SetName(resourceName);
    }

    uint8_t* MapCpuWriteOnly()
    {
        uint8_t* mappedData{};
        // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        DX::ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
        return mappedData;
    }
};

// Helper class to create and update a constant buffer with proper constant buffer alignments.
// Usage: ToDo
//    ConstantBuffer<...> cb;
//    cb.Create(...);
//    cb.staging.var = ...; | cb->var = ... ; 
//    cb.CopyStagingToGPU(...);
template <class T>
class ConstantBuffer : public GpuUploadBuffer
{
    uint8_t* m_mappedConstantData;
    UINT m_alignedInstanceSize;
    UINT m_numInstances;

public:
    ConstantBuffer() : m_alignedInstanceSize(0), m_numInstances(0), m_mappedConstantData(nullptr) {}

    void Create(ID3D12Device* device, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
    {
        m_numInstances = numInstances;
        UINT alignedSize = Align(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        UINT bufferSize = numInstances * alignedSize;
        Allocate(device, bufferSize, resourceName);
        m_mappedConstantData = MapCpuWriteOnly();
        m_alignedInstanceSize = alignedSize; // This was missing!!!
    }

    void CopyStagingToGpu(UINT instanceIndex = 0)
    {
        memcpy(m_mappedConstantData + instanceIndex * m_alignedInstanceSize, &staging, sizeof(T));
    }

    // Accessors
    T staging;
    T* operator->() { return &staging; }
    UINT NumInstances() { return m_numInstances; }
    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
    {
        return m_resource->GetGPUVirtualAddress() + UINT64(instanceIndex * m_alignedInstanceSize);
    }
};

// Helper class to create and update a structured buffer.
// Usage: 
//    StructuredBuffer<...> sb;
//    sb.Create(...);
//    sb[index].var = ... ; 
//    sb.CopyStagingToGPU(...);
//    Set...View(..., sb.GputVirtualAddress());
template <class T>
class StructuredBuffer : public GpuUploadBuffer
{
    T* m_mappedBuffers;
    std::vector<T> m_staging;
    UINT m_numInstances;

public:
    // Performance tip: Align structures on sizeof(float4) boundary.
    // Ref: https://developer.nvidia.com/content/understanding-structured-buffer-performance
    static_assert(sizeof(T) % 16 == 0, L"Align structure buffers on 16 byte boundary for performance reasons.");

    StructuredBuffer() : m_mappedBuffers(nullptr), m_numInstances(0) {}

    void Create(ID3D12Device10* device, UINT numElements, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
    {
        m_numInstances = numInstances;  // This was missing!
        m_staging.resize(numElements);
        UINT bufferSize = UINT64(numInstances * numElements) * sizeof(T);
        AllocateWithLayout(device, bufferSize, resourceName);
        //Allocate(device, bufferSize, resourceName);
        m_mappedBuffers = reinterpret_cast<T*>(MapCpuWriteOnly());
    }

    void CopyStagingToGpu(UINT instanceIndex = 0)
    {
        // This is the correct memcpy operation.
        // Using the incorrect memcpy has cost me almost a week of debugging pain.
        // Please update the stackoverflow article with the solution.
        memcpy(m_mappedBuffers + instanceIndex * NumElementsPerInstance(), &m_staging[0], InstanceSize());
        //memcpy(m_mappedBuffers + instanceIndex, &m_staging[0], InstanceSize());
    }

    // Accessors
    T& operator[](UINT elementIndex) { return m_staging[elementIndex]; }
    size_t NumElementsPerInstance() { return m_staging.size(); }
    UINT NumInstances() { return m_numInstances; }
    size_t InstanceSize() { return NumElementsPerInstance() * sizeof(T); }
    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
    {
        return m_resource->GetGPUVirtualAddress() + instanceIndex * InstanceSize();
    }
};

struct D3DBuffer
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
};

// Execute indirect resources.
// Data structure to match the command signature used for ExecuteIndirect.
struct IndirectCommand
{
    D3D12_GPU_VIRTUAL_ADDRESS cbv;
    D3D12_DRAW_INDEXED_ARGUMENTS drawIndexedArguments;
    //D3D12_DRAW_ARGUMENTS drawArguments;
};

// For use with calls to DrawIndexedInstance().
struct ProcGeometryBuffersAndViews
{
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW               vertexBufferView;
    D3D12_VERTEX_BUFFER_VIEW               transformInstanceBufferView; // Instanced geometry only.
    //D3D12_VERTEX_BUFFER_VIEW               materialInstanceBufferView; // Instanced geometry only.
    D3D12_INDEX_BUFFER_VIEW                indexBufferView;
    uint32_t                               indexCountPerInstance;
};

// Added these convenience functions.

inline void CreateByteAddressShaderResourceView(
    ID3D12Device* device,
    ID3D12Resource* buffer,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor)
{
#if defined(_MSC_VER) || !defined(_WIN32)
    const auto desc = buffer->GetDesc();
#else
    D3D12_RESOURCE_DESC tmpDesc;
    const auto& desc = *buffer->GetDesc(&tmpDesc);
#endif

    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER
        || (desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0)
    {
        OutputDebugStringA("ERROR: CreateBufferShaderResourceView called on an unsupported resource.\n");
        //DebugTrace("ERROR: CreateBufferShaderResourceView called on an unsupported resource.\n");
        throw std::runtime_error("invalid buffer resource");
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS; // Raw buffers do not support _r16 typeless format.
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = static_cast<uint32_t>(desc.Width / 4); // Raw buffer element size is 4 bytes.
    srvDesc.Buffer.StructureByteStride = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    device->CreateShaderResourceView(buffer, &srvDesc, srvDescriptor);
}

inline void CreateBufferShaderResourceViewPerFrameIndex(
    ID3D12Device* device,
    ID3D12Resource* buffer,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    uint32_t numElements,
    uint32_t stride,
    size_t firstElement)
{
#if defined(_MSC_VER) || !defined(_WIN32)
    const auto desc = buffer->GetDesc();
#else
    D3D12_RESOURCE_DESC tmpDesc;
    const auto& desc = *buffer->GetDesc(&tmpDesc);
#endif

    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER
        || (desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0)
    {
        OutputDebugStringA("ERROR: CreateBufferShaderResourceView called on an unsupported resource.\n");
        throw std::runtime_error("invalid buffer resource");
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = firstElement;
    // .Buffer.NumElements specifies the number of elements of the view into the buffer, NOT the total elements of the buffer!
    srvDesc.Buffer.NumElements = numElements;
    //srvDesc.Buffer.NumElements = (stride > 0)
    //    ? static_cast<UINT>(desc.Width / stride)
    //    : static_cast<UINT>(desc.Width);
    srvDesc.Buffer.StructureByteStride = stride;

    device->CreateShaderResourceView(buffer, &srvDesc, srvDescriptor);
}

inline void CreateTLASBufferShaderResourceView(
    ID3D12Device* device,
    //ID3D12Resource* buffer,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddess,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = gpuAddess;
    device->CreateShaderResourceView(nullptr, &srvDesc, srvDescriptor);
}

// Shorthand for creating a versioned root signature
inline HRESULT CreateVersionedRootSignature(
    ID3D12Device* device,
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootSignatureDesc,
    D3D_ROOT_SIGNATURE_VERSION version,
    ID3D12RootSignature** rootSignature)
{
    Microsoft::WRL::ComPtr<ID3DBlob> pSignature;
    Microsoft::WRL::ComPtr<ID3DBlob> pError;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(rootSignatureDesc, version, &pSignature, &pError);
    if (SUCCEEDED(hr))
    {
        hr = device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(),
            IID_GRAPHICS_PPV_ARGS(rootSignature)
        );
    }
    return hr;
}

inline void CreateGraphicsPipelineStateOnWorkerThread(
    const DirectX::EffectPipelineStateDescription& pipelineStateDesc,
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    const D3D12_SHADER_BYTECODE& vertexShader,
    const D3D12_SHADER_BYTECODE& pixelShader,
    ID3D12PipelineState** pPipelineState)
{
    pipelineStateDesc.CreatePipelineState(device, rootSignature, vertexShader, pixelShader, pPipelineState);
}

inline void CreateComputePipelineStateOnWorkerThread(
    ID3D12Device* device,
    const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc,
    ID3D12PipelineState** pPipelineState)
{
    DX::ThrowIfFailed(device->CreateComputePipelineState(pDesc, IID_PPV_ARGS(pPipelineState)));
}
