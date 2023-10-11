#pragma once

class OcclusionManager
{
public:

    OcclusionManager(
        ID3D12Device10* device,
        ID3D12PipelineState* horzBlurPso,
        ID3D12PipelineState* vertBlurPso,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav,
        uint32_t cbvSrvUavDescriptorSize,
        bool isAcquiringNormalDepthBuffer,
        bool isLargeBlurRadius) noexcept;

    OcclusionManager(OcclusionManager const&) = delete;
    OcclusionManager& operator= (OcclusionManager const&) = delete;

    OcclusionManager(OcclusionManager&&) = default;
    OcclusionManager& operator= (OcclusionManager&&) = default;

    virtual ~OcclusionManager() = default;

protected:

    static constexpr DXGI_FORMAT OcclusionFormat   = DXGI_FORMAT_R8_UNORM;
    static constexpr DXGI_FORMAT NormalDepthFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

    //static const uint32_t MaxBlurRadius = 5;

    ID3D12Device10* m_d3dDevice;    // Enhanced Barriers support.

    ID3D12PipelineState* m_blurHorzPso;
    ID3D12PipelineState* m_blurVertPso;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_occlusionBuffer0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_occlusionBuffer1;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalDepthBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_prevFrameBuffer;   // To provide temporal filtering.

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hOcclusionBuffer0CpuUav;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hOcclusionBuffer1CpuUav;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalDepthBufferCpuUav;

    // SRVs are required for filtered sampling in shaders.
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hOcclusionBuffer0CpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalDepthBufferCpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hPrevFrameBufferCpuSrv;

    int m_bufferWidth;
    int m_bufferHeight;

    std::vector<float> m_weights;

    bool m_isNormalDepthBufferAcquired;
    bool m_isLargeBlurRadius;

public:

    void OnResize(int newWidth, int newHeight);
    void CopyPreviousFrameBuffer(ID3D12GraphicsCommandList7* commandList);
    void Denoise(ID3D12GraphicsCommandList7* commandList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, int blurCount);

    const auto GetBufferWidth()  const noexcept { return m_bufferWidth; };
    const auto GetBufferHeight() const noexcept { return m_bufferHeight; };

    const auto GetBlurWeights() const noexcept { return m_weights.data(); }

private:

    void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuUav, uint32_t cbvSrvUavDescriptorSize);

    void CalcGaussWeights();

    // Blurs the occlusion buffer to smooth out the noise caused by only taking a few random samples per pixel.
    // We use an edge preserving blur so that we do not blur across discontinuities--we want edges to remain edges.
    // A bilateral blur is achieved by "ping-ponging" intermediate blur results between two buffers.
    void BlurOcclusionBuffer(ID3D12GraphicsCommandList7* commandList, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, uint32_t blurCount);

    void PerformBlurPass(ID3D12GraphicsCommandList7* commandList, bool isHorzBlur);

    void BuildResources();

    void SetUavEnhancedBarrier(ID3D12Resource* buffer, ID3D12GraphicsCommandList7* commandList);
};
