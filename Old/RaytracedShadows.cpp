//***************************************************************************************
// RtShadow.cpp
//***************************************************************************************

#include "pch.h"
#include "RaytracedShadows.h"

using namespace DirectX;
using namespace DX;

RaytracedShadows::RaytracedShadows(
    ID3D12Device* device,
    ID3D12PipelineState* horzBlurPso,
    ID3D12PipelineState* vertBlurPso,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
    uint32_t cbvSrvUavDescriptorSize) noexcept :
    m_d3dDevice(device),
    m_blurHorzPso(horzBlurPso),
    m_blurVertPso(vertBlurPso),
    m_renderTargetHeight(0), m_renderTargetWidth(0)
{
    BuildDescriptors(hCpuUav, cbvSrvUavDescriptorSize);
    CalcGaussWeights();
}

void RaytracedShadows::CalcGaussWeights()
//void RaytracedShadows::CalcGaussWeights(float sigma)
//std::vector<float> RaytracedShadows::CalcGaussWeights(float sigma)
{
    const float sigma = 1.25f;
    const float twoSigma2 = 2.f * sigma * sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    const int blurRadius = static_cast<int>(ceil(2.f * sigma)); // Do not change to uint... must be int for negative values!

    assert(blurRadius <= RaytracedShadows::MaxBlurRadius);

    //std::vector<float> weights;
    m_weights.resize(static_cast<size_t>(2 * blurRadius + 1));

    float weightSum = 0;
    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = static_cast<float>(i);

        m_weights[static_cast<size_t>(i + blurRadius)] = expf(-x * x / twoSigma2);

        weightSum += m_weights[static_cast<size_t>(i + blurRadius)];
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

void RaytracedShadows::BuildDescriptors(
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
    uint32_t cbvSrvUavDescriptorSize)
{
    // Save references to the descriptors for three contiguous UAVs.

    m_hShadowMap0CpuUav = hCpuUav;
    m_hShadowMap1CpuUav = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);
    m_hShadowMap0CpuSrv = hCpuUav.Offset(1, cbvSrvUavDescriptorSize); // Added this.
}

void RaytracedShadows::OnResize(int newWidth, int newHeight)
{
    if (m_renderTargetWidth != newWidth || m_renderTargetHeight != newHeight)
    {
        m_renderTargetWidth = newWidth;
        m_renderTargetHeight = newHeight;

        BuildResources();
    }
}

void RaytracedShadows::Denoise(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, int blurCount)
{
    // The RTAO method no longer requires its own root signature, as the application now binds a general compute rootsig
    // shared by all dispatches.
    // The general compute rootsig should already have been set for the compute skinning dispatch prior to raytracing.

    // Transition resources in preparation for blur.
    // Edit: replaced by UAV barriers that ensure UAV writes are completed before next access as read resources.
    //D3D12_RESOURCE_BARRIER* preBlurBarrier = {};
    const auto preBlurBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_shadowMap0.Get());
    commandList->ResourceBarrier(1, &preBlurBarrier);

    BlurShadowMap(commandList, gpuAddress, blurCount);
}

void RaytracedShadows::BlurShadowMap(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
    uint32_t blurCount)
{
    commandList->SetComputeRootConstantBufferView(0, gpuAddress);

    for (uint32_t i = 0; i < blurCount; ++i)
    {
        BlurShadowMap(commandList, true);
        BlurShadowMap(commandList, false);
    }
}

void RaytracedShadows::BlurShadowMap(ID3D12GraphicsCommandList* commandList, bool isHorzBlur)
{
    // How many groups do we need to dispatch to cover a row of pixels, where each group covers 256 pixels?
    // (256 is defined in the ComputeShader)
    int  numGroupsX, numGroupsY;

    // Ping-pong the two shadow map textures as we apply horizontal and vertical blur passes.
    if (isHorzBlur == true)
    {
        commandList->SetPipelineState(m_blurHorzPso);

        numGroupsX = static_cast<int>(ceilf(static_cast<float>(m_renderTargetWidth) / 256.f));
        numGroupsY = m_renderTargetHeight;
    }
    else
    {
        commandList->SetPipelineState(m_blurVertPso);

        numGroupsX = m_renderTargetWidth;
        numGroupsY = static_cast<int>(ceilf(static_cast<float>(m_renderTargetHeight) / 256.f));
    }

    commandList->Dispatch(numGroupsX, numGroupsY, 1);

    // Flip resource transitions for next blur stage.
    // Edit: replaced by UAV barriers to change uav access from write/read & read/write.
    D3D12_RESOURCE_BARRIER flipBarriers[2] = {};
    flipBarriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_shadowMap0.Get());
    flipBarriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_shadowMap1.Get());
    commandList->ResourceBarrier(2, flipBarriers);
}

void RaytracedShadows::BuildResources()
{
    // Create the output resource.
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        RaytracedShadows::ShadowFormat,
        m_renderTargetWidth, m_renderTargetHeight,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr /*& optClear*/,
        IID_PPV_ARGS(&m_shadowMap0)
    ));

    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr /*& optClear*/,
        IID_PPV_ARGS(&m_shadowMap1)
    ));

    // New DXTK methods.
    CreateUnorderedAccessView(m_d3dDevice, m_shadowMap0.Get(), m_hShadowMap0CpuUav);
    CreateUnorderedAccessView(m_d3dDevice, m_shadowMap1.Get(), m_hShadowMap1CpuUav);

    CreateShaderResourceView(m_d3dDevice, m_shadowMap0.Get(), m_hShadowMap0CpuSrv);
}
