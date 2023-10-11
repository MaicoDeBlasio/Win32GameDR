//***************************************************************************************
// Rtao.h by Frank Luna (C) 2015 All Rights Reserved.
// Edit: Converted to support real-time ambient occlusion and renamed Rtao.h
//***************************************************************************************

#pragma once

//#include "../../Common/d3dUtil.h"
//#include "d3dUtil.h"
//#include "FrameResource.h"

class RaytracedAO
{
public:

    RaytracedAO(
        ID3D12Device* device,
        //ID3D12RootSignature* computeSig,
        ID3D12PipelineState* horzBlurPso,
        ID3D12PipelineState* vertBlurPso,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
        uint32_t cbvSrvUavDescriptorSize) noexcept;
    //Rtao(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) noexcept; // temp constructor overload

    // Constructor also incorporates root sig and PSO initialization.
    //Rtao(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig,
    //    ID3D12PipelineState* ssaoPso,
    //    ID3D12PipelineState* ssaoBlurPso) noexcept;
    //UINT width, UINT height);

    RaytracedAO(const RaytracedAO& rhs) = delete;
    RaytracedAO& operator=(const RaytracedAO& rhs) = delete;

    RaytracedAO(RaytracedAO&&) = default;
    RaytracedAO& operator= (RaytracedAO&&) = default;

    ~RaytracedAO() = default;

    // Ambient term is a low-precision value between 0-1.
    static const DXGI_FORMAT AmbientOcclusionFormat = DXGI_FORMAT_R8_UNORM;
    //static const DXGI_FORMAT AmbientOcclusionFormat = DXGI_FORMAT_R16_UNORM;
    //static const DXGI_FORMAT AmbientOcclusionFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static const DXGI_FORMAT NormalDepthFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    static const uint32_t MaxBlurRadius = 5;
    //static const int MaxBlurRadius = 5;

    //void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);

    //auto AmbientMap() { return m_ambientMap0.Get(); };
    //ID3D12Resource* NormalMap();
    //ID3D12Resource* AmbientMap();

    //CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv()const;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv()const;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE AmbientMapSrv()const;

    //void RebuildDescriptors();
    //void RebuildDescriptors(ID3D12Resource* depthStencilBuffer);

    //void SetRootSigAndPSOs(ID3D12RootSignature* computeSig,
    // ID3D12PipelineState* horzBlurPso, ID3D12PipelineState* vertBlurPso);
    //void SetPSOs(ID3D12PipelineState* ssaoPso, ID3D12PipelineState* ssaoBlurPso);

    ///<summary>
    /// Call when the backbuffer is resized.  
    ///</summary>
    void OnResize(int newWidth, int newHeight);

    ///<summary>
    /// Changes the render target to the Ambient render target and draws a fullscreen
    /// quad to kick off the pixel shader to compute the AmbientMap.  We still keep the
    /// main depth buffer binded to the pipeline, but depth buffer read/writes
    /// are disabled, as we do not need the depth buffer computing the Ambient map.
    ///</summary>
    //void ComputeSsao(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
    //    //FrameResource* currFrame, 
    //    int blurCount);

    void Denoise(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
        //FrameResource* currFrame, 
        int blurCount);

    const auto GetBufferWidth()  const noexcept { return m_renderTargetWidth; };
    const auto GetBufferHeight() const noexcept { return m_renderTargetHeight; };

    const auto GetBlurWeights() const noexcept { return m_weights.data(); }

private:

    void BuildDescriptors(
        //ID3D12Resource* depthStencilBuffer, 
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
        //CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuUav,
        //CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
        uint32_t cbvSrvUavDescriptorSize);
    //UINT rtvDescriptorSize);

    void CalcGaussWeights();
    //void CalcGaussWeights(float sigma = 2.5f);
    //std::vector<float> CalcGaussWeights(float sigma);

    ///<summary>
    /// Blurs the ambient map to smooth out the noise caused by only taking a
    /// few random samples per pixel.  We use an edge preserving blur so that 
    /// we do not blur across discontinuities--we want edges to remain edges.
    ///</summary>
    void BlurAmbientMap(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
        uint32_t blurCount);
    //void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame, int blurCount);

    void BlurAmbientMap(ID3D12GraphicsCommandList* commandList, bool isHorzBlur);

    void BuildResources();
    //void BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList);

    //void BuildOffsetVectors();


private:

    ID3D12Device* m_d3dDevice;
    //Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;

    //ID3D12RootSignature* m_rootSig;
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
    //Microsoft::WRL::ComPtr<ID3D12RootSignature> mSsaoRootSig;

    ID3D12PipelineState* m_blurHorzPso;
    ID3D12PipelineState* m_blurVertPso;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_blurHorzPso;
    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_blurVertPso;

    //Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMap;
    //Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMapUploadBuffer;
    //Microsoft::WRL::ComPtr<ID3D12Resource> mNormalMap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ambientMap0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_ambientMap1;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalDepth; // added this

    // added these
    //std::unique_ptr<std::mt19937> mRandEng;
    //std::uniform_real_distribution<FLOAT> mDist;

    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuSrv;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuRtv;

    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhDepthMapCpuSrv;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhDepthMapGpuSrv;

    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv;

    // Need two for ping-ponging during blur.
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap0CpuUav;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap0GpuUav;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap0CpuSrv; // Required for bilinear sampling in dxr shader.
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap0GpuSrv;
    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap1CpuUav;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap1GpuUav;
    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuSrv;
    //CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap1GpuSrv;
    //CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalDepthCpuUav; // Added this.
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalDepthCpuSrv; // Added this.

    int m_renderTargetWidth;
    int m_renderTargetHeight;

    std::vector<float> m_weights;   // Added this.

    //DirectX::XMFLOAT4 mOffsets[14];

    //D3D12_VIEWPORT mViewport;
    //D3D12_RECT mScissorRect;
};
