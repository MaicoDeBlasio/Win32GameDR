#include "pch.h"
#include "DXSampleHelper.h"
#include "OcclusionManager.h"

using namespace DirectX;
using namespace DX;

OcclusionManager::OcclusionManager(
    ID3D12Device10* device,
    ID3D12PipelineState* horzBlurPso,
    ID3D12PipelineState* vertBlurPso,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
    uint32_t cbvSrvUavDescriptorSize,
    bool isAcquiringNormalDepth,
    bool isLargeBlurRadius) noexcept :
    m_d3dDevice(device),
    m_blurHorzPso(horzBlurPso),
    m_blurVertPso(vertBlurPso),
    m_isNormalDepthBufferAcquired(isAcquiringNormalDepth),
    m_isLargeBlurRadius(isLargeBlurRadius),
    m_bufferHeight(0), m_bufferWidth(0)
{
    BuildDescriptors(hCpuUav, cbvSrvUavDescriptorSize);
    CalcGaussWeights();
}

void OcclusionManager::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav, uint32_t cbvSrvUavDescriptorSize)
{
    m_hOcclusionBuffer0CpuUav = hCpuUav;
    m_hOcclusionBuffer1CpuUav = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);
    m_hOcclusionBuffer0CpuSrv = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);

    if (m_isNormalDepthBufferAcquired)
    {
        m_hNormalDepthBufferCpuUav = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);
        m_hNormalDepthBufferCpuSrv = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);
    }

    m_hPrevFrameBufferCpuSrv = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);
}

void OcclusionManager::CalcGaussWeights()
{
    const int blurRadius = m_isLargeBlurRadius ? 5 : 3;
    const float sigma = static_cast<float>(blurRadius) / 2.f;
    const float twoSigma2 = 2.f * sigma * sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // The width of the bell curve is
    // const int blurRadius = static_cast<int>(ceil(2.f * sigma)); // Do not change to uint... must be int for negative values!

    //assert(blurRadius <= RaytracedAO::MaxBlurRadius);

    //std::vector<float> weights;
    m_weights.resize(static_cast<size_t>(2 * blurRadius + 1));
    //weights.resize(static_cast<size_t>(2 * blurRadius + 1));
    //weights.resize(2 * blurRadius + 1);

    float weightSum = 0;

    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = static_cast<float>(i);

        m_weights[static_cast<size_t>(i + blurRadius)] = expf(-x * x / twoSigma2);

        weightSum += m_weights[static_cast<size_t>(i + blurRadius)];
        //weightSum += weights[static_cast<size_t>(i + blurRadius)];
        //weightSum += weights[i + blurRadius];
    }

    // Divide by the sum so all the weights add up to 1.0.
    for (auto& weight : m_weights)
    {
        weight /= weightSum;
    }
    //for (uint32_t i = 0; i < weights.size(); ++i)
    //{
    //    weights[i] /= weightSum;
    //}

    //return weights;
}

void OcclusionManager::BuildResources()
{
    // Create the output resource.
    auto texDesc = CD3DX12_RESOURCE_DESC1::Tex2D(
        OcclusionManager::OcclusionFormat,
        m_bufferWidth, m_bufferHeight,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource3(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
        //D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        /*& optClear,*/ nullptr, nullptr, 0, nullptr,
        IID_PPV_ARGS(&m_occlusionBuffer0)
    ));
    NAME_D3D12_OBJECT(m_occlusionBuffer0);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource3(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
        //D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        /*& optClear,*/ nullptr, nullptr, 0, nullptr,
        IID_PPV_ARGS(&m_occlusionBuffer1)
    ));
    NAME_D3D12_OBJECT(m_occlusionBuffer1);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource3(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,
        //D3D12_RESOURCE_STATE_COMMON,
        /*& optClear,*/ nullptr, nullptr, 0, nullptr,
        IID_PPV_ARGS(&m_prevFrameBuffer)
    ));
    NAME_D3D12_OBJECT(m_prevFrameBuffer);

    // New DXTK methods.
    CreateUnorderedAccessView(m_d3dDevice, m_occlusionBuffer0.Get(), m_hOcclusionBuffer0CpuUav);
    CreateUnorderedAccessView(m_d3dDevice, m_occlusionBuffer1.Get(), m_hOcclusionBuffer1CpuUav);
    CreateShaderResourceView(m_d3dDevice,  m_occlusionBuffer0.Get(), m_hOcclusionBuffer0CpuSrv);
    CreateShaderResourceView(m_d3dDevice,  m_prevFrameBuffer.Get(),  m_hPrevFrameBufferCpuSrv);

    if (m_isNormalDepthBufferAcquired)
    {
        texDesc.Format = OcclusionManager::NormalDepthFormat;
        ThrowIfFailed(m_d3dDevice->CreateCommittedResource3(
            //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,
            //D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            /*& optClear,*/ nullptr, nullptr, 0, nullptr,
            IID_PPV_ARGS(&m_normalDepthBuffer)
        ));
        NAME_D3D12_OBJECT(m_normalDepthBuffer);

        CreateUnorderedAccessView(m_d3dDevice, m_normalDepthBuffer.Get(), m_hNormalDepthBufferCpuUav);
        CreateShaderResourceView(m_d3dDevice,  m_normalDepthBuffer.Get(), m_hNormalDepthBufferCpuSrv);
    }
}

void OcclusionManager::OnResize(int newWidth, int newHeight)
{
    if (m_bufferWidth != newWidth || m_bufferHeight != newHeight)
    {
        m_bufferWidth = newWidth;
        m_bufferHeight = newHeight;

        BuildResources();
    }
}

void OcclusionManager::CopyPreviousFrameBuffer(ID3D12GraphicsCommandList7* commandList)
{
    // Save existing pixels in the occlusion buffer to the 'previous frame' buffer to allow temporal filtering.
    const auto occlusionBuffer = m_occlusionBuffer0.Get();

    D3D12_TEXTURE_BARRIER barrierUnorderedAccessToCopySource[] =
    {
        CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_COMPUTE_SHADING,     // SyncBefore
            D3D12_BARRIER_SYNC_COPY,     // SyncAfter
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,  // AccessBefore
            D3D12_BARRIER_ACCESS_COPY_SOURCE,  // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,  // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,  // LayoutAfter
            occlusionBuffer,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),       // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        )
    };
    D3D12_BARRIER_GROUP barrierGroupUnorderedAccessToCopySource[] =
    {
        CD3DX12_BARRIER_GROUP(1, barrierUnorderedAccessToCopySource)
    };
    commandList->Barrier(1, barrierGroupUnorderedAccessToCopySource);
    //auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(occlusionBuffer,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    //commandList->ResourceBarrier(1, &barrier);

    commandList->CopyResource(m_prevFrameBuffer.Get(), occlusionBuffer);

    D3D12_TEXTURE_BARRIER barrierCopySourceToUnorderedAccess[] =
    {
        CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_COPY,     // SyncBefore
            D3D12_BARRIER_SYNC_COMPUTE_SHADING,     // SyncAfter
            D3D12_BARRIER_ACCESS_COPY_SOURCE,  // AccessBefore
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,  // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,  // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,  // LayoutAfter
            occlusionBuffer,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),       // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        )
    };
    D3D12_BARRIER_GROUP barrierGroupCopySourceToUnorderedAccess[] =
    {
        CD3DX12_BARRIER_GROUP(1, barrierCopySourceToUnorderedAccess)
    };
    commandList->Barrier(1, barrierGroupCopySourceToUnorderedAccess);
    //barrier = CD3DX12_RESOURCE_BARRIER::Transition(occlusionBuffer,
    //    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //commandList->ResourceBarrier(1, &barrier);
}

void OcclusionManager::Denoise(ID3D12GraphicsCommandList7* commandList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, int blurCount)
{
    // The RTAO method no longer requires its own root signature, as the application now binds a general compute rootsig
    // shared by all dispatches.
    // The general compute rootsig should already have been set for the compute skinning dispatch prior to raytracing.

    // Set root signature for Rtao blur
    //commandList->SetComputeRootSignature(m_rootSig);
    //commandList->SetComputeRootSignature(m_rootSig.Get());

    // Transition resources in preparation for blur.
    // Edit: replaced by UAV barriers that ensure UAV writes are completed before next access as read resources.
    // Edit: UAV legacy barriers have been replaced with the equivalent enhanced barrier.
    SetUavEnhancedBarrier(m_occlusionBuffer0.Get(), commandList);
    //const auto preBlurOcclusionBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_occlusionBuffer0.Get());
    //commandList->ResourceBarrier(1, &preBlurOcclusionBarrier);

    if (m_isNormalDepthBufferAcquired)
    {
        SetUavEnhancedBarrier(m_normalDepthBuffer.Get(), commandList);
        //const auto preBlurNormalDepthBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_normalDepthBuffer.Get());
        //commandList->ResourceBarrier(1, &preBlurNormalDepthBarrier);
    }

    BlurOcclusionBuffer(commandList, gpuAddress, blurCount);
}

void OcclusionManager::BlurOcclusionBuffer(ID3D12GraphicsCommandList7* commandList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
    uint32_t blurCount)
{
    commandList->SetComputeRootConstantBufferView(0, gpuAddress);

    for (uint32_t i = 0; i < blurCount; ++i)
    {
        PerformBlurPass(commandList, true);
        PerformBlurPass(commandList, false);
    }
}

void OcclusionManager::PerformBlurPass(ID3D12GraphicsCommandList7* commandList, bool isHorzBlur)
{
    // How many groups do we need to dispatch to cover a row of pixels, where each group covers 64 pixels?
    // (64 is defined in the ComputeShader)
    int  numGroupsX, numGroupsY;

    // Ping-pong the two occlusion textures as we apply horizontal and vertical blur passes.
    if (isHorzBlur == true)
    {
        commandList->SetPipelineState(m_blurHorzPso);

        numGroupsX = static_cast<int>(ceilf(static_cast<float>(m_bufferWidth) / 64.f));
        numGroupsY = m_bufferHeight;
    }
    else
    {
        commandList->SetPipelineState(m_blurVertPso);

        numGroupsX = m_bufferWidth;
        numGroupsY = static_cast<int>(ceilf(static_cast<float>(m_bufferHeight) / 64.f));
    }

    commandList->Dispatch(numGroupsX, numGroupsY, 1);

    // Flip resource transitions for next blur stage.
    // Edit: replaced by UAV barriers to change uav access from write/read & read/write.
    // Edit: UAV legacy barriers have been replaced with the equivalent enhanced barriers.
    SetUavEnhancedBarrier(m_occlusionBuffer0.Get(), commandList);
    SetUavEnhancedBarrier(m_occlusionBuffer1.Get(), commandList);
    //D3D12_RESOURCE_BARRIER flipBarriers[2] = {};
    //flipBarriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_occlusionBuffer0.Get());
    //flipBarriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_occlusionBuffer1.Get());
    //commandList->ResourceBarrier(2, flipBarriers);
}

void OcclusionManager::SetUavEnhancedBarrier(ID3D12Resource* buffer, ID3D12GraphicsCommandList7* commandList)
{
    D3D12_TEXTURE_BARRIER uavBarrier = CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_COMPUTE_SHADING,     // SyncBefore
            D3D12_BARRIER_SYNC_COMPUTE_SHADING,     // SyncAfter
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,  // AccessBefore
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,  // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,  // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,  // LayoutAfter
            buffer,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),       // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        );
    D3D12_BARRIER_GROUP uavBarrierGroup = CD3DX12_BARRIER_GROUP(1, &uavBarrier);
    commandList->Barrier(1, &uavBarrierGroup);
}
