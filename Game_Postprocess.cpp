//
// Game_Postprocess.cpp
//

// This method handles all scene postprocessing prior to presentation.

#include "pch.h"
#include "Game.h"

void Game::DoPostprocessing(ID3D12GraphicsCommandList7* commandList)
{
    // Save a copy of the original HDR texture.
    //const auto rtvHdrCopyDescriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::HdrCopyRtv);
    //commandList->OMSetRenderTargets(1, &rtvHdrCopyDescriptor, false, nullptr);

    //auto postProcess = m_basicPostProc[PostProcessType::Copy].get();

    //m_renderTex[RenderTextures::HdrCopy]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::HdrSrv), m_renderTex[RenderTextures::Hdr]->GetResource());
    //postProcess->Process(commandList);
    //m_renderTex[RenderTextures::HdrCopy]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Resize viewport to quarter resolution.
    const auto vp = m_deviceResources->GetScreenViewport();
    auto halfvp   = Viewport(vp);
    halfvp.height /= 2.f;
    halfvp.width  /= 2.f;
    commandList->RSSetViewports(1, halfvp.Get12());

    // Bloom extract to halfsize render target & viewport.
    const auto rtvPass1Descriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::Pass1Rtv);
    commandList->OMSetRenderTargets(1, &rtvPass1Descriptor, false, nullptr);

    m_renderTex[RenderTextures::Pass1]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mPass1RenderTex->BeginScene(commandList); // transition to render target state
    auto postProcess = m_basicPostProc[PostProcessType::BloomExtract].get();

    postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::HdrSrv), m_renderTex[RenderTextures::Hdr]->GetResource());
    postProcess->SetBloomExtractParameter(0.25f);
    postProcess->Process(commandList);
    m_renderTex[RenderTextures::Pass1]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mPass1RenderTex->EndScene(commandList); // transition to pixel shader resource state

    // Bloom horizontal blur
    const auto rtvPass2Descriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::Pass2Rtv);
    commandList->OMSetRenderTargets(1, &rtvPass2Descriptor, false, nullptr);

    m_renderTex[RenderTextures::Pass2]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mPass2RenderTex->BeginScene(commandList); // transition to render target state
    postProcess = m_basicPostProc[PostProcessType::BloomBlur].get();

    postProcess->SetBloomBlurParameters(true, 4.f, 1.f);
    postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::Pass1Srv), m_renderTex[RenderTextures::Pass1]->GetResource());
    postProcess->Process(commandList);
    m_renderTex[RenderTextures::Pass2]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mPass2RenderTex->EndScene(commandList); // transition to pixel shader resource state

    // Bloom vertical blur
    commandList->OMSetRenderTargets(1, &rtvPass1Descriptor, false, nullptr);

    m_renderTex[RenderTextures::Pass1]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mPass1RenderTex->BeginScene(commandList); // transition to render target state

    postProcess->SetBloomBlurParameters(false, 4.f, 1.f);
    postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::Pass2Srv), m_renderTex[RenderTextures::Pass2]->GetResource());
    postProcess->Process(commandList);
    m_renderTex[RenderTextures::Pass1]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mPass1RenderTex->EndScene(commandList); // transition to pixel shader resource state

    // Bloom combine
    const auto rtvBloomDescriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::BloomRtv);
    commandList->OMSetRenderTargets(1, &rtvBloomDescriptor, false, nullptr);

    m_renderTex[RenderTextures::Bloom]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mBloomRenderTex->BeginScene(commandList); // transition to render target state
    const auto dualProcess = m_bloomCombine.get();

    dualProcess->SetBloomCombineParameters(1.25f, 1.f, 1.f, 1.f);
    dualProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::HdrSrv));
    dualProcess->SetSourceTexture2(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::Pass1Srv));
    dualProcess->Process(commandList);
    m_renderTex[RenderTextures::Bloom]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mBloomRenderTex->EndScene(commandList); // transition to pixel shader resource state

    // Luna blur filter on compute shader

    //// blur filter expects the input resource to be in RT state
    //mHdrRenderTex->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mBlurFilter->Execute(commandList, mComputeRootSig.Get(), mPsoBlurHorz.Get(), mPsoBlurVert.Get(), mHdrRenderTex->GetResource(), 4u);
    //// transition Hdr resource back to RT state for final image render
    //commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mHdrRenderTex->GetResource(),
    //    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
    //
    //// Prepare to copy blurred output to the blur texture render target.
    //
    //mBlurRenderTex->TransitionTo(commandList, D3D12_RESOURCE_STATE_COPY_DEST);
    //commandList->CopyResource(mBlurRenderTex->GetResource(), mBlurFilter->Output());
    //mBlurRenderTex->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //
    ////commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurRenderTex->GetResource(),
    ////    D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

    // Blur postprocess.
    commandList->OMSetRenderTargets(1, &rtvPass2Descriptor, false, nullptr);

    m_renderTex[RenderTextures::Pass2]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mPass2RenderTex->BeginScene(commandList); // transition to render target state
    postProcess = m_basicPostProc[PostProcessType::Copy].get();

    postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::HdrSrv), m_renderTex[RenderTextures::Hdr]->GetResource());
    postProcess->Process(commandList);
    m_renderTex[RenderTextures::Pass2]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //mPass2RenderTex->EndScene(commandList); // transition to pixel shader resource

    uint32_t blurPasses = 2;
    postProcess = m_basicPostProc[PostProcessType::Blur].get();

    for (uint32_t i = 0; i < blurPasses; ++i)
    {
        // Horizontal blur
        commandList->OMSetRenderTargets(1, &rtvPass1Descriptor, false, nullptr);

        m_renderTex[RenderTextures::Pass1]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        //mPass1RenderTex->BeginScene(commandList); // transition to render target state
        postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::Pass2Srv), m_renderTex[RenderTextures::Pass2]->GetResource());
        postProcess->SetGaussianParameter(1.f);
        postProcess->SetBloomBlurParameters(true, 4.f, 1.f); // horizontal blur
        postProcess->Process(commandList);
        m_renderTex[RenderTextures::Pass1]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        //mPass1RenderTex->EndScene(commandList); // transition to pixel shader resource

        // Vertical blur
        //CD3DX12_CPU_DESCRIPTOR_HANDLE rtvBlurDescriptor(mRtvDescHeap->GetCpuHandle(RTV_Descriptors::BlurRTV));
        commandList->OMSetRenderTargets(1, &rtvPass2Descriptor, false, nullptr);

        m_renderTex[RenderTextures::Pass2]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        //mPass2RenderTex->BeginScene(commandList); // transition to render target state
        //mBlurRenderTex->BeginScene(commandList); // transition to render target state
        postProcess->SetSourceTexture(m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::Pass1Srv), m_renderTex[RenderTextures::Pass1]->GetResource());
        postProcess->SetBloomBlurParameters(false, 4.f, 1.f); // vertical blur
        postProcess->Process(commandList);
        m_renderTex[RenderTextures::Pass2]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        //mPass2RenderTex->EndScene(commandList); // transition to pixel shader resource
    }

    // Render the final image to the original hdr texture & fullscreen viewport.
    //auto rtvHdrDescriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::HdrRtv);
    //commandList->OMSetRenderTargets(1, &rtvHdrDescriptor, false, nullptr);

    //SetDebugObjectName(m_renderTex[RenderTextures::Hdr]->GetResource(), L"HdrTexture");
    commandList->RSSetViewports(1, &vp); // Resize viewport to fullscreen size.
    //m_renderTex[RenderTextures::Hdr]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    //mHdrRenderTex->BeginScene(commandList); // transition to render target state

    // Preprocessors changed the graphics root signature, so rebind our app's graphics root signature.
    // Edit: We are now using a compute shader to post-process the final scene, and the compute rootsig is still bound
    // if we are using the DXR pipeline.
    // We are also using the DXR resource as the output since we require a UAV to write to, and it is free to use. 
    if (m_mainScene->GetRasterFlag())
        commandList->SetComputeRootSignature(m_rootSig[RootSignatures::Compute].Get());

    commandList->SetPipelineState(m_pipelineState[PSOs::PostProcess].Get());

    // Calculate thread groups and dispatch compute shader.
    // Thread number N defined as 8 in shader.
    // Submit dispatch to graphics command list because this is not an asynch dispatch.
    int numGroupsX = static_cast<int>(ceilf(static_cast<float>(m_width) / 8.f));
    int numGroupsY = static_cast<int>(ceilf(static_cast<float>(m_height) / 8.f));

    commandList->Dispatch(numGroupsX, numGroupsY, 1);

    // State transition the raytracing output resource so it can be used as the source texture in the tone-mapper.
    const auto raytracingOutput = m_raytracingOutput.Get();
    D3D12_TEXTURE_BARRIER barrierUnorderedAccessToPixelShader =
        CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_ALL_SHADING,                 // SyncBefore
            D3D12_BARRIER_SYNC_PIXEL_SHADING,               // SyncAfter
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,          // AccessBefore
            D3D12_BARRIER_ACCESS_SHADER_RESOURCE,           // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,          // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,                    // LayoutAfter
            raytracingOutput,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),  // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        );
    D3D12_BARRIER_GROUP barrierGroupUnorderedAccessToPixelShader =
        CD3DX12_BARRIER_GROUP(1, &barrierUnorderedAccessToPixelShader);
    commandList->Barrier(1, &barrierGroupUnorderedAccessToPixelShader);
    //auto dxrBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(),
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    //);
    //ScopedBarrier scopedBarrier(commandList, &dxrBarrier, 1); // Reverses the transition when we exit this scope.

    // Pass projection values to pixel shader.
    // See Zink p.522
    //
    // A = FarClipDist / (FarClipDist - NearClipDist)
    // B = -FarClipDist * NearClipDist / (FarClipDist - NearClipDist)
    //
    // Values for A and B are found in the projection matrix P at P(2,2) and P(3,2):
    // | x 0 0 0 |
    // | 0 y 0 0 |
    // | 0 0 A 1 |
    // | 0 0 B 0 |

    //PSDistortionPassConstants psDistortionConst{};
    //psDistortionConst.gDistortion = mFireDistortionValue;
    //GraphicsResource psDistortionCB = mGraphicsMemory->AllocateConstant(psDistortionConst);

    //PSDofPassConstants dofPassConstants = {};

    //dofPassConstants.projValues.x = cameraProj(2, 2); // proj value A (negated in shader for RH view system)
    //dofPassConstants.projValues.y = cameraProj(3, 2); // proj value B
    //dofPassConstants.distortion = mFireValue;

    //Matrix projMat = mGameCamera.GetProj();
    //dofPassConstants.invProjMatrix = projMat.Invert().Transpose();
    //Matrix invProjMat;
    //projMat.Invert(invProjMat);
    //invProjMat.Transpose(dofPassConstants.invProjMatrix);
    //dofPassConstants.projectionValues.x = +1.f; // approaches unity as NearClipDist decreases
    //dofPassConstants.projectionValues.y = -1.f; // approaches -NearClipDist as NearClipDist decreases
    ////dofPassConstants.projectionValues.x = +1000.f / 999.9f; // approaches unity as NearClipDist decreases
    ////dofPassConstants.projectionValues.y = -1000.f / 999.9f; // approaches -NearClipDist as NearClipDist decreases
    //dofPassConstants.dofRangeValues.x = 10.f;  // DOF start value in world units
    //dofPassConstants.dofRangeValues.y = 1.f; // DOF reciprocal range value in world units

    //auto psDofCB = mGraphicsMemory->AllocateConstant(dofPassConstants);
    //GraphicsResource psDofCB = mGraphicsMemory->AllocateConstant(mFireDistortionValue);

    //commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::PSDistortionCB, psDistortionCB.GpuAddress());
    //commandList->SetGraphicsRootConstantBufferView(RootParameterIndex::PSDofCB, psDofCB.GpuAddress());
    //commandList->SetGraphicsRootDescriptorTable(RootParameterIndex::PostProcDT, mPostProcSrvDescHeap->GetFirstGpuHandle());

    // set screen coords (NDC)

    //VertexPositionTexture qtop_left(Vector3(-1.f, -1.f, 0.f), Vector2(0.f, 1.f));
    //VertexPositionTexture qtop_right(Vector3(1.f, -1.f, 0.f), Vector2(1.f, 1.f));
    //VertexPositionTexture qbot_right(Vector3(1.f, 1.f, 0.f), Vector2(1.f, 0.f));
    //VertexPositionTexture qbot_left(Vector3(-1.f, 1.f, 0.f), Vector2(0.f, 0.f));
    //
    //m_quad_batch->Begin(commandList);
    //m_quad_batch->DrawQuad(qtop_left, qtop_right, qbot_right, qbot_left);
    //m_quad_batch->End();

    //mHdrRenderTex->EndScene(commandList);

    // Set swapchain backbuffer as the tonemapping render target.
    // Edit: We are now tonemapping to another render texture.
    const auto rtvTonemapDescriptor = m_descHeap[DescriptorHeaps::Rtv]->GetCpuHandle(RTVs::TonemapRtv);
    //auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvTonemapDescriptor, false, nullptr);

    m_renderTex[RenderTextures::Tonemap]->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Set texture descriptor heap in prep for postprocessing/tone mapping.
    //ID3D12DescriptorHeap* srvUavHeap[] = { m_srvUavHeap->Heap() };
    //commandList->SetDescriptorHeaps(UINT(std::size(srvUavHeap)), srvUavHeap);

    // Edit: GPU-based validation is triggered if the HDR texture isn't transitioned to 'pixel shader resource' state
    // when it is the source texture for the tonemapping process.
    //m_renderTex[RenderTextures::Hdr]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //m_hdrTex->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto hdrDescriptor = m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::DxrSrv);
    //auto hdrDescriptor = m_descHeap[DescriptorHeaps::SrvUav]->GetGpuHandle(SrvUAVs::HdrSrv);

    // The tonemapper binds its own graphics root signature.
    ApplyToneMapping(commandList, hdrDescriptor);

    m_renderTex[RenderTextures::Tonemap]->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // State transition the raytracing output resource using Enhanced Barriers.
    D3D12_TEXTURE_BARRIER barrierShaderSourceToUnorderedAccess =
        CD3DX12_TEXTURE_BARRIER(
            D3D12_BARRIER_SYNC_PIXEL_SHADING,               // SyncBefore
            D3D12_BARRIER_SYNC_ALL_SHADING,                 // SyncAfter
            D3D12_BARRIER_ACCESS_SHADER_RESOURCE,           // AccessBefore
            D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,          // AccessAfter
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON,                    // LayoutBefore
            D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS,          // LayoutAfter
            raytracingOutput,
            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),  // All subresources
            D3D12_TEXTURE_BARRIER_FLAG_NONE
        );
    D3D12_BARRIER_GROUP barrierGroupShaderSourceToUnorderedAccess =
        CD3DX12_BARRIER_GROUP(1, &barrierShaderSourceToUnorderedAccess);
    commandList->Barrier(1, &barrierGroupShaderSourceToUnorderedAccess);

    // FXAA pass.

    // Set swapchain backbuffer as the FXAA pass render target.
    const auto rtvBackbufferDescriptor = m_deviceResources->GetRenderTargetView();
    commandList->OMSetRenderTargets(1, &rtvBackbufferDescriptor, false, nullptr);

    // Reset the graphics signature and set the PSO for the FXAA pass.
    commandList->SetGraphicsRootSignature(m_rootSig[RootSignatures::Graphics].Get());
    commandList->SetPipelineState(m_pipelineState[PSOs::Fxaa].Get());

    // Use the big triangle optimization to draw a fullscreen quad.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);
}

void Game::ApplyToneMapping(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE hdrDescriptor)
{
    ///// Use switch block for HDR10 displays /////

    switch (m_deviceResources->GetColorSpace())
    {
    default:
        m_toneMap[TonemapType::Default]->SetHDRSourceTexture(hdrDescriptor);
        m_toneMap[TonemapType::Default]->Process(commandList);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        m_toneMap[TonemapType::HDR10]->SetHDRSourceTexture(hdrDescriptor);
        m_toneMap[TonemapType::HDR10]->Process(commandList);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        // Required if R16G16B16A16_FLOAT is used as display format (otherwise you can omit this case).
        m_toneMap[TonemapType::Linear]->SetHDRSourceTexture(hdrDescriptor);
        m_toneMap[TonemapType::Linear]->Process(commandList);
        break;
    }
}
