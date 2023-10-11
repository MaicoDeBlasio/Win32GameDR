//***************************************************************************************
// RtShadow.h
//***************************************************************************************

#pragma once

class RaytracedShadows
{
public:

    RaytracedShadows(
        ID3D12Device* device,
        ID3D12PipelineState* horzBlurPso,
        ID3D12PipelineState* vertBlurPso,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
        uint32_t cbvSrvUavDescriptorSize) noexcept;

    RaytracedShadows(const RaytracedShadows& rhs) = delete;
    RaytracedShadows& operator=(const RaytracedShadows& rhs) = delete;

    RaytracedShadows(RaytracedShadows&&) = default;
    RaytracedShadows& operator= (RaytracedShadows&&) = default;

    ~RaytracedShadows() = default;

    // Shadow term is a low-precision value between 0-1.
    static const DXGI_FORMAT ShadowFormat = DXGI_FORMAT_R8_UNORM;

    static const uint32_t MaxBlurRadius = 3;

    //auto ShadowMap() { return m_shadowMap0.Get(); };

    ///<summary>
    /// Call when the backbuffer is resized.  
    ///</summary>
    void OnResize(int newWidth, int newHeight);

    void Denoise(ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
        int blurCount);

    const auto GetBufferWidth()  const noexcept { return m_renderTargetWidth; };
    const auto GetBufferHeight() const noexcept { return m_renderTargetHeight; };

    const auto GetBlurWeights() const noexcept { return m_weights.data(); }

private:

    void BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
        uint32_t cbvSrvUavDescriptorSize);

    void CalcGaussWeights();
    //void CalcGaussWeights(float sigma = 1.25f);
    //std::vector<float> CalcGaussWeights(float sigma);

    ///<summary>
    /// Blurs the shadow map to smooth out the noise caused by only taking a
    /// few random samples per pixel.  We use an edge preserving blur so that 
    /// we do not blur across discontinuities--we want edges to remain edges.
    ///</summary>
    void BlurShadowMap(
        ID3D12GraphicsCommandList* commandList,
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
        uint32_t blurCount);

    void BlurShadowMap(ID3D12GraphicsCommandList* commandList, bool isHorzBlur);

    void BuildResources();

private:

    ID3D12Device* m_d3dDevice;

    ID3D12PipelineState* m_blurHorzPso;
    ID3D12PipelineState* m_blurVertPso;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_shadowMap0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_shadowMap1;

    // Need two for ping-ponging during blur.
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hShadowMap0CpuUav;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hShadowMap0CpuSrv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hShadowMap1CpuUav; // required for bilinear sampling in dxr shader

    int m_renderTargetWidth;
    int m_renderTargetHeight;

    std::vector<float> m_weights;   // Added this.
};
