//***************************************************************************************
// Rtao.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "pch.h"
#include "RaytracedAO.h"
//#include <DirectXPackedVector.h>

using namespace DirectX;
//using namespace DirectX::PackedVector; // XMCOLOR struct
using namespace DX;

//namespace HeapVariables
//{
//    XMCOLOR initData[256 * 256]{}; // declared globally to avoid stack overflow warning
//}

//Rtao::Rtao(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) noexcept :
RaytracedAO::RaytracedAO(
    ID3D12Device* device,
    //ID3D12RootSignature* computeSig,
    ID3D12PipelineState* horzBlurPso,
    ID3D12PipelineState* vertBlurPso,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
    uint32_t cbvSrvUavDescriptorSize) noexcept :
    m_d3dDevice(device),
    //m_rootSig(computeSig),
    m_blurHorzPso(horzBlurPso),
    m_blurVertPso(vertBlurPso),
    m_renderTargetHeight(0), m_renderTargetWidth(0)
{
    BuildDescriptors(hCpuUav, cbvSrvUavDescriptorSize);
    CalcGaussWeights();
    //BuildOffsetVectors();
    //BuildRandomVectorTexture(cmdList);
}

//Rtao::Rtao(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
// ID3D12RootSignature* rootSig, ID3D12PipelineState* ssaoPso,
//    ID3D12PipelineState* ssaoBlurPso) noexcept :
//    //UINT width, UINT height)
//    md3dDevice(device),
//    mRootSig(rootSig),
//    mSsaoPso(ssaoPso),
//    mBlurPso(ssaoBlurPso)
//{
//    // We will call OnResize() from Game::CreateWindowDependentResources()
//    //OnResize(width, height);
//
//    //BuildOffsetVectors();
//    //BuildRandomVectorTexture(cmdList);
//}

//uint32_t Rtao::RtaoMapWidth()const
//{
//    return m_renderTargetWidth / 2;
//}
//
//uint32_t Rtao::RtaoMapHeight()const
//{
//    return m_renderTargetHeight / 2;
//}

//void Rtao::GetOffsetVectors(DirectX::XMFLOAT4 offsets[14])
//{
//    std::copy(&mOffsets[0], &mOffsets[14], &offsets[0]);
//}

void RaytracedAO::CalcGaussWeights()
//void RaytracedAO::CalcGaussWeights(float sigma)
//std::vector<float> RaytracedAO::CalcGaussWeights(float sigma = 2.5f)
{
    const float sigma = 2.5f;
    const float twoSigma2 = 2.f * sigma * sigma;

    // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
    // For example, for sigma = 3, the width of the bell curve is 
    const int blurRadius = static_cast<int>(ceil(2.f * sigma)); // Do not change to uint... must be int for negative values!

    assert(blurRadius <= RaytracedAO::MaxBlurRadius);

    //std::vector<float> weights;
    m_weights.resize(static_cast<size_t>(2 * blurRadius + 1));
    //weights.resize(static_cast<size_t>(2 * blurRadius + 1));
    //weights.resize(2 * blurRadius + 1);

    float weightSum = 0;

    for (int i = -blurRadius; i <= blurRadius; ++i)
    {
        float x = static_cast<float>(i);

        m_weights[static_cast<size_t>(i + blurRadius)] = expf(-x * x / twoSigma2);
        //weights[static_cast<size_t>(i + blurRadius)] = expf(-x * x / twoSigma2);
        //weights[i + blurRadius] = expf(-x * x / twoSigma2);

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

//ID3D12Resource* Rtao::NormalMap()
//{
//    return mNormalMap.Get();
//}

//ID3D12Resource* Rtao::AmbientMap()
//{
//    return m_ambientMap0.Get();
//}

//CD3DX12_CPU_DESCRIPTOR_HANDLE Rtao::NormalMapRtv()const
//{
//    return mhNormalMapCpuRtv;
//}
//
//CD3DX12_GPU_DESCRIPTOR_HANDLE Rtao::NormalMapSrv()const
//{
//    return mhNormalMapGpuSrv;
//}
//
//CD3DX12_GPU_DESCRIPTOR_HANDLE Rtao::AmbientMapSrv()const
//{
//    return mhAmbientMap0GpuSrv;
//}

void RaytracedAO::BuildDescriptors(
    //ID3D12Resource* depthStencilBuffer,
    CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
    //CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuUav,
    //CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
    uint32_t cbvSrvUavDescriptorSize)
    //UINT rtvDescriptorSize)
{
    // Save references to the descriptors for three contiguous UAVs.

    //mhAmbientMap0CpuSrv = hCpuSrv;
    m_hAmbientMap0CpuUav = hCpuUav;
    //mhAmbientMap1CpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    m_hAmbientMap1CpuUav = hCpuUav.Offset(1, cbvSrvUavDescriptorSize);
    //mhNormalMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    //mhDepthMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    //mhRandomVectorMapCpuSrv = hCpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    //mhAmbientMap0GpuUav = hGpuUav;
    //mhAmbientMap0GpuSrv = hGpuSrv;
    //mhAmbientMap1GpuUav = hGpuUav.Offset(1, cbvSrvUavDescriptorSize);
    //mhAmbientMap1GpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    //mhNormalMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    //mhDepthMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);
    //mhRandomVectorMapGpuSrv = hGpuSrv.Offset(1, cbvSrvUavDescriptorSize);

    //mhNormalMapCpuRtv = hCpuRtv;
    //mhAmbientMap0CpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);
    //mhAmbientMap1CpuRtv = hCpuRtv.Offset(1, rtvDescriptorSize);

    m_hAmbientMap0CpuSrv = hCpuUav.Offset(1, cbvSrvUavDescriptorSize); // added this
    m_hNormalDepthCpuUav = hCpuUav.Offset(1, cbvSrvUavDescriptorSize); // added this
    m_hNormalDepthCpuSrv = hCpuUav.Offset(1, cbvSrvUavDescriptorSize); // added this

    //  Create the descriptors
    //BuildResources();
    //RebuildDescriptors();
    //RebuildDescriptors(depthStencilBuffer);
}

/*
void Rtao::RebuildDescriptors()
//void Rtao::RebuildDescriptors(ID3D12Resource* depthStencilBuffer)
{
    //D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    //srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    //srvDesc.Format = NormalMapFormat;
    //srvDesc.Texture2D.MostDetailedMip = 0;
    //srvDesc.Texture2D.MipLevels = 1;
    //md3dDevice->CreateShaderResourceView(mNormalMap.Get(), &srvDesc, mhNormalMapCpuSrv);

    // We have created the depth pass srv independently of the Rtao class.

    //srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    //srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // to be updated
    //md3dDevice->CreateShaderResourceView(depthStencilBuffer, &srvDesc, mhDepthMapCpuSrv);

    //srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //md3dDevice->CreateShaderResourceView(mRandomVectorMap.Get(), &srvDesc, mhRandomVectorMapCpuSrv);

    //srvDesc.Format = AmbientMapFormat;
    //md3dDevice->CreateShaderResourceView(mAmbientMap0.Get(), &srvDesc, mhAmbientMap0CpuSrv);
    //md3dDevice->CreateShaderResourceView(mAmbientMap1.Get(), &srvDesc, mhAmbientMap1CpuSrv);

    //D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    //uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    // Must respecify format or debug layer error 'resource is not fully typed' occurs.
    //uavDesc.Format = Rtao::AmbientOcclusionFormat;
    //uavDesc.Texture2D.MipSlice = 0u;

    //m_d3dDevice->CreateUnorderedAccessView(m_ambientMap0.Get(), nullptr, &uavDesc, m_hAmbientMap0CpuUav);
    //m_d3dDevice->CreateUnorderedAccessView(m_ambientMap1.Get(), nullptr, &uavDesc, m_hAmbientMap1CpuUav);
    //uavDesc.Format = Rtao::NormalDepthFormat;
    //m_d3dDevice->CreateUnorderedAccessView(m_normalDepth.Get(), nullptr, &uavDesc, m_hNormalDepthCpuUav);

    //D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    //rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    //rtvDesc.Format = NormalMapFormat;
    //rtvDesc.Texture2D.MipSlice = 0;
    //rtvDesc.Texture2D.PlaneSlice = 0;
    //md3dDevice->CreateRenderTargetView(mNormalMap.Get(), &rtvDesc, mhNormalMapCpuRtv);

    //rtvDesc.Format = AmbientMapFormat;
    //md3dDevice->CreateRenderTargetView(mAmbientMap0.Get(), &rtvDesc, mhAmbientMap0CpuRtv);
    //md3dDevice->CreateRenderTargetView(mAmbientMap1.Get(), &rtvDesc, mhAmbientMap1CpuRtv);
}
*/

//void Rtao::SetRootSigAndPSOs(ID3D12RootSignature* computeSig,
// ID3D12PipelineState* horzBlurPso, ID3D12PipelineState* vertBlurPso)
//{
//    m_rootSig = computeSig;
//    m_blurHorzPso = horzBlurPso;
//    m_blurVertPso = vertBlurPso;
//}

void RaytracedAO::OnResize(int newWidth, int newHeight)
{
    if (m_renderTargetWidth != newWidth || m_renderTargetHeight != newHeight)
    {
        m_renderTargetWidth = newWidth;
        m_renderTargetHeight = newHeight;

        // We render to ambient map at half the resolution.
        //mViewport.TopLeftX = 0.0f;
        //mViewport.TopLeftY = 0.0f;
        //mViewport.Width = mRenderTargetWidth / 2.0f;
        //mViewport.Height = mRenderTargetHeight / 2.0f;
        //mViewport.MinDepth = 0.0f;
        //mViewport.MaxDepth = 1.0f;

        //mScissorRect = { 0, 0, (int)mRenderTargetWidth / 2, (int)mRenderTargetHeight / 2 };

        BuildResources();
        //RebuildDescriptors(); // Relocated this call from Game::CreateWindowSizeDependentResources()
    }
}

void RaytracedAO::Denoise(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, int blurCount)
{
    // The RTAO method no longer requires its own root signature, as the application now binds a general compute rootsig
    // shared by all dispatches.
    // The general compute rootsig should already have been set for the compute skinning dispatch prior to raytracing.

    // Set root signature for Rtao blur
    //commandList->SetComputeRootSignature(m_rootSig);
    //commandList->SetComputeRootSignature(m_rootSig.Get());

    // Transition resources in preparation for blur.
    // Edit: replaced by UAV barriers that ensure UAV writes are completed before next access as read resources.
    D3D12_RESOURCE_BARRIER preBlurBarriers[2] = {};
    preBlurBarriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_ambientMap0.Get());
    preBlurBarriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_normalDepth.Get());
    //preBlurBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
    // D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    //preBlurBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(mNormalDepth.Get(),
    // D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(2, preBlurBarriers);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    //D3D12_RESOURCE_STATE_RENDER_TARGET,
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    //    //D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    BlurAmbientMap(commandList, gpuAddress, blurCount);
    //BlurAmbientMap(cmdList, currFrame, blurCount);
}

/*
void Rtao::ComputeSsao(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, int blurCount)
//void Rtao::ComputeSsao(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame, int blurCount)
{
    cmdList->RSSetViewports(1, &mViewport);
    cmdList->RSSetScissorRects(1, &mScissorRect);

    // We compute the initial Rtao to AmbientMap0.

    // Change to RENDER_TARGET.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
        //D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET));

    float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    cmdList->ClearRenderTargetView(mhAmbientMap0CpuRtv, clearValue, 0, nullptr);

    // Specify the buffers we are going to render to.
    cmdList->OMSetRenderTargets(1, &mhAmbientMap0CpuRtv, true, nullptr);

    // Set root signature for Rtao blur
    cmdList->SetGraphicsRootSignature(mRootSig);

    // Bind the constant buffer for this pass.
    cmdList->SetGraphicsRootConstantBufferView(0, gpuAddress);

    //auto ssaoCBAddress = currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    //cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

    cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

    // Bind the normal and depth maps.
    cmdList->SetGraphicsRootDescriptorTable(2, mhNormalMapGpuSrv);

    // Bind the random vector map.
    cmdList->SetGraphicsRootDescriptorTable(3, mhRandomVectorMapGpuSrv);

    cmdList->SetPipelineState(mSsaoPso);

    // Draw fullscreen quad.
    cmdList->IASetVertexBuffers(0, 0, nullptr);
    cmdList->IASetIndexBuffer(nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(6, 1, 0, 0);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        //D3D12_RESOURCE_STATE_GENERIC_READ));
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    BlurAmbientMap(cmdList, gpuAddress, blurCount);
    //BlurAmbientMap(cmdList, currFrame, blurCount);
}
*/

void RaytracedAO::BlurAmbientMap(
    ID3D12GraphicsCommandList* commandList,
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
    uint32_t blurCount)
//void Rtao::BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame, int blurCount)
{
    //cmdList->SetComputeRootSignature(mRootSig);
    //cmdList->SetPipelineState(mBlurPso);

    commandList->SetComputeRootConstantBufferView(0, gpuAddress);
    //cmdList->SetGraphicsRootConstantBufferView(0, gpuAddress);

    //auto ssaoCBAddress = currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    //cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

    for (uint32_t i = 0; i < blurCount; ++i)
    {
        BlurAmbientMap(commandList, true);
        BlurAmbientMap(commandList, false);
    }

    // Flip resource transitions for next blur stage.
    // Edit: replaced by UAV barriers to change uav access from write/read & read/write.
    //D3D12_RESOURCE_BARRIER flipBarriers[2] = {};
    //flipBarriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_ambientMap0.Get());
    //flipBarriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_ambientMap1.Get());
    //commandList->ResourceBarrier(2, flipBarriers);

    // Transition resources to correct state before returning to the caller.
    //D3D12_RESOURCE_BARRIER postBlurBarriers[2] = {};
    //postBlurBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_ambientMap0.Get(),
    // D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //postBlurBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_normalDepth.Get(),
    // D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //commandList->ResourceBarrier(2, postBlurBarriers);

    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mAmbientMap0.Get(),
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    //    //D3D12_RESOURCE_STATE_COPY_SOURCE));
}

void RaytracedAO::BlurAmbientMap(ID3D12GraphicsCommandList* commandList, bool isHorzBlur)
{
    //ID3D12Resource* input = nullptr;
    //ID3D12Resource* output = nullptr;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE inputUav{}, outputUav{};
    //CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

    // How many groups do we need to dispatch to cover a row of pixels, where each group covers 256 pixels?
    // (256 is defined in the ComputeShader)
    int  numGroupsX, numGroupsY;

    // Ping-pong the two ambient map textures as we apply
    // horizontal and vertical blur passes.
    if (isHorzBlur == true)
    {
        commandList->SetPipelineState(m_blurHorzPso);
        //commandList->SetPipelineState(m_blurHorzPso.Get());

        //input = mAmbientMap0.Get();
        //output = mAmbientMap1.Get();
        //inputUav = mhAmbientMap0GpuUav;
        //outputUav = mhAmbientMap1GpuUav;
        //outputRtv = mhAmbientMap1CpuRtv;

        numGroupsX = static_cast<int>(ceilf(static_cast<float>(m_renderTargetWidth) / 256.f));
        numGroupsY = m_renderTargetHeight;
        //numGroupsX = (UINT)ceilf(mRenderTargetWidth / 256.f);
        //numGroupsY = mRenderTargetHeight;

        //cmdList->SetComputeRoot32BitConstant(1, 1, 0);
        //cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
    }
    else
    {
        commandList->SetPipelineState(m_blurVertPso);
        //commandList->SetPipelineState(m_blurVertPso.Get());

        //input = mAmbientMap1.Get();
        //output = mAmbientMap0.Get();
        //inputUav = mhAmbientMap1GpuUav;
        //outputUav = mhAmbientMap0GpuUav;
        //outputRtv = mhAmbientMap0CpuRtv;

        numGroupsX = m_renderTargetWidth;
        numGroupsY = static_cast<int>(ceilf(static_cast<float>(m_renderTargetHeight) / 256.f));
        //numGroupsX = mRenderTargetWidth;
        //numGroupsY = (UINT)ceilf(mRenderTargetHeight / 256.f);

        //cmdList->SetComputeRoot32BitConstant(1, 0, 0);
        //cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
    }

    commandList->Dispatch(numGroupsX, numGroupsY, 1);

    // Flip resource transitions for next blur stage.
    // Edit: replaced by UAV barriers to change uav access from write/read & read/write.
    D3D12_RESOURCE_BARRIER flipBarriers[2] = {};
    flipBarriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_ambientMap0.Get());
    flipBarriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_ambientMap1.Get());
    //flipBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(input,
    // D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    //flipBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(output,
    // D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(2, flipBarriers);

    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    //
    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
    //    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    //    D3D12_RESOURCE_STATE_RENDER_TARGET));

    //float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //cmdList->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);

    //cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

    // Normal/depth map still bound.


    // Bind the normal and depth maps.
    //cmdList->SetGraphicsRootDescriptorTable(2, mhNormalMapGpuSrv);

    // Bind the input ambient map to second texture table.
    //cmdList->SetGraphicsRootDescriptorTable(3, inputSrv);

    // Draw fullscreen quad.
    //cmdList->IASetVertexBuffers(0, 0, nullptr);
    //cmdList->IASetIndexBuffer(nullptr);
    //cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //cmdList->DrawInstanced(6, 1, 0, 0);

    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
    //    D3D12_RESOURCE_STATE_RENDER_TARGET,
    //    D3D12_RESOURCE_STATE_GENERIC_READ));
    //    //D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void RaytracedAO::BuildResources()
{
    //auto device = m_d3dDevice.Get();

    // Free the old resources if they exist.
    //mNormalMap = nullptr;
    //mAmbientMap0 = nullptr;
    //mAmbientMap1 = nullptr;

    // Create the output resource.
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        RaytracedAO::AmbientOcclusionFormat,
        m_renderTargetWidth, m_renderTargetHeight,
        //Rtao::AmbientMapFormat,
        //mRenderTargetWidth, mRenderTargetHeight,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    //D3D12_RESOURCE_DESC texDesc;
    //ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    //texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    //texDesc.Alignment = 0;
    //texDesc.Width = mRenderTargetWidth;
    //texDesc.Height = mRenderTargetHeight;
    //texDesc.DepthOrArraySize = 1;
    //texDesc.MipLevels = 1;
    ////texDesc.Format = Rtao::NormalMapFormat;
    //texDesc.SampleDesc.Count = 1;
    //texDesc.SampleDesc.Quality = 0;
    //texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    //texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    //float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    //float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    //CD3DX12_CLEAR_VALUE optClear(NormalMapFormat, normalClearColor);
    //ThrowIfFailed(md3dDevice->CreateCommittedResource(
    //    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    //    D3D12_HEAP_FLAG_NONE,
    //    &texDesc,
    //    //D3D12_RESOURCE_STATE_GENERIC_READ,
    //    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    //    &optClear,
    //    IID_PPV_ARGS(&mNormalMap)));

    // Ambient occlusion maps are at quarter resolution.
    //texDesc.Width = mRenderTargetWidth / 2;
    //texDesc.Height = mRenderTargetHeight / 2;
    //texDesc.Format = Rtao::AmbientMapFormat;

    //float ambientClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    //CD3DX12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(AmbientMapFormat, ambientClearColor);
    //optClear = CD3DX12_CLEAR_VALUE(AmbientMapFormat, ambientClearColor);

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        //D3D12_RESOURCE_STATE_RENDER_TARGET,
        //D3D12_RESOURCE_STATE_GENERIC_READ,
        //D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr /*& optClear*/,
        IID_PPV_ARGS(&m_ambientMap0)
    ));

    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        //D3D12_RESOURCE_STATE_RENDER_TARGET,
        //D3D12_RESOURCE_STATE_GENERIC_READ,
        //D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr /*& optClear*/,
        IID_PPV_ARGS(&m_ambientMap1)
    ));

    texDesc.Format = RaytracedAO::NormalDepthFormat;
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        //&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        &defaultHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        //D3D12_RESOURCE_STATE_RENDER_TARGET,
        //D3D12_RESOURCE_STATE_GENERIC_READ,
        //D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr /*& optClear*/,
        IID_PPV_ARGS(&m_normalDepth)
    ));

    // New DXTK methods.
    CreateUnorderedAccessView(m_d3dDevice, m_ambientMap0.Get(), m_hAmbientMap0CpuUav);
    CreateUnorderedAccessView(m_d3dDevice, m_ambientMap1.Get(), m_hAmbientMap1CpuUav);
    CreateUnorderedAccessView(m_d3dDevice, m_normalDepth.Get(), m_hNormalDepthCpuUav);

    CreateShaderResourceView(m_d3dDevice, m_ambientMap0.Get(), m_hAmbientMap0CpuSrv);
    CreateShaderResourceView(m_d3dDevice, m_normalDepth.Get(), m_hNormalDepthCpuSrv);
}

/*
void Rtao::BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList)
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = 256;
    texDesc.Height = 256;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    //texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        //D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&mRandomVectorMap)));

    //
    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap.
    //

    const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mRandomVectorMap.Get(), 0, num2DSubresources);

    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        // resources on D3D12_HEAP_TYPE_UPLOAD heaps require generic read state
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(mRandomVectorMapUploadBuffer.GetAddressOf())));

    //XMCOLOR initData[256 * 256]{};

    mDist = std::uniform_real_distribution<float>(0.f, 1.f);

    for (int i = 0; i < 256; ++i)
    {
        for (int j = 0; j < 256; ++j)
        {
            // Random vector in [0,1].  We will decompress in shader to [-1,1].
            XMFLOAT3 v(mDist(*mRandEng), mDist(*mRandEng), mDist(*mRandEng));
            //XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

            HeapVariables::initData[i * 256 + j] = XMCOLOR(v.x, v.y, v.z, 0.0f);
        }
    }

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = HeapVariables::initData;
    subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
    subResourceData.SlicePitch = subResourceData.RowPitch * 256;

    //
    // Schedule to copy the data to the default resource, and change states.
    // Note that mCurrSol is put in the GENERIC_READ state so it can be
    // read by a shader.
    //

    //cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRandomVectorMap.Get(),
    //    D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources(cmdList, mRandomVectorMap.Get(), mRandomVectorMapUploadBuffer.Get(),
        0, 0, num2DSubresources, &subResourceData);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRandomVectorMap.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        //D3D12_RESOURCE_STATE_GENERIC_READ));
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void Rtao::BuildOffsetVectors()
{
    // This function is called from class constructor before BuildRandomVectorTexture(),
    // so initialize random number engine here.

    std::random_device rd;
    mRandEng = std::make_unique<std::mt19937>(rd());
    mDist = std::uniform_real_distribution<float>(0.25f, 1.f);

    // Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
    // and the 6 center points along each cube face.  We always alternate the points on
    // opposites sides of the cubes.  This way we still get the vectors spread out even
    // if we choose to use less than 14 samples.

    // 8 cube corners
    mOffsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
    mOffsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

    mOffsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
    mOffsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

    mOffsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
    mOffsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

    mOffsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
    mOffsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

    // 6 centers of cube faces
    mOffsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
    mOffsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

    mOffsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
    mOffsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

    mOffsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
    mOffsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

    for (int i = 0; i < 14; ++i)
    {
        // Create random lengths in [0.25, 1.0].

        //auto s = mDist(*mRandEng);
        //float s = MathHelper::RandF(0.25f, 1.0f);

        XMVECTOR v = XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));
        //XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&mOffsets[i]));

        XMStoreFloat4(&mOffsets[i], v);
    }
}
*/
