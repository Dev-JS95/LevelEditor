#pragma once
#include "FrameResource.h"
#include <vector>

class Ssao
{
    //������
public:
    Ssao(ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        UINT width, UINT height);
    Ssao(const Ssao& rhs) = delete;
    Ssao& operator=(const Ssao& rhs) = delete;
    ~Ssao() = default;


    //�޼ҵ�
public:
    UINT SsaoMapWidth()const;
    UINT SsaoMapHeight()const;

    void GetOffsetVectors(DirectX::XMFLOAT4 offsets[14]);
    std::vector<float> CalcGaussWeights(float sigma);

    ID3D12Resource* NormalMap();
    ID3D12Resource* AmbientMap();

    CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv()const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE AmbientMapSrv()const;

    void BuildDescriptors(
        ID3D12Resource* depthStencilBuffer,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
        UINT cbvSrvUavDescriptorSize,
        UINT rtvDescriptorSize);

    void RebuildDescriptors(ID3D12Resource* depthStencilBuffer);

    void SetPSOs(ID3D12PipelineState* ssaoPso, ID3D12PipelineState* ssaoBlurPso);

    ///<summary>
    /// Call when the backbuffer is resized.  
    ///</summary>
    void OnResize(UINT newWidth, UINT newHeight);

    ///<summary>
    /// Changes the render target to the Ambient render target and draws a fullscreen
    /// quad to kick off the pixel shader to compute the AmbientMap.  We still keep the
    /// main depth buffer binded to the pipeline, but depth buffer read/writes
    /// are disabled, as we do not need the depth buffer computing the Ambient map.
    ///</summary>
    void ComputeSsao(
        ID3D12GraphicsCommandList* cmdList,
        FrameResource* currFrame,
        int blurCount);

private:

    ///<summary>
    /// Blurs the ambient map to smooth out the noise caused by only taking a
    /// few random samples per pixel.  We use an edge preserving blur so that 
    /// we do not blur across discontinuities--we want edges to remain edges.
    ///</summary>
    void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, FrameResource* currFrame, int blurCount);
    void BlurAmbientMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur);
    void BuildResources();
    void BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList);
    void BuildOffsetVectors();

    //�ʵ�
public:
    static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
    static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    static const int MaxBlurRadius = 5; 

private:
    ID3D12Device* md3dDevice;

    ATL::CComPtr<ID3D12RootSignature> mSsaoRootSig;

    ID3D12PipelineState* mSsaoPso = nullptr;
    ID3D12PipelineState* mBlurPso = nullptr;

    ATL::CComPtr<ID3D12Resource> mRandomVectorMap;
    ATL::CComPtr<ID3D12Resource> mRandomVectorMapUploadBuffer;
    ATL::CComPtr<ID3D12Resource> mNormalMap;
    ATL::CComPtr<ID3D12Resource> mAmbientMap0;
    ATL::CComPtr<ID3D12Resource> mAmbientMap1;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhNormalMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhNormalMapCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhDepthMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhDepthMapGpuSrv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhRandomVectorMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhRandomVectorMapGpuSrv;

    // Need two for ping-ponging during blur.
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap0GpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap0CpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mhAmbientMap1GpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mhAmbientMap1CpuRtv;

    UINT mRenderTargetWidth;
    UINT mRenderTargetHeight;

    DirectX::XMFLOAT4 mOffsets[14];

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;
};


