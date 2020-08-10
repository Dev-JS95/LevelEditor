#pragma once
#include "SSAO.h"
#include "D3D12ResourceType.h"
#include "Camera.h"
#include "../Core/GameTimer.h"


#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


/*Direct3D12를 사용하는 렌더러*/
class Direct3D12Renderer {
	//생성자
public:
	Direct3D12Renderer(HANDLE hWnd) : 
		mhMainWnd((HWND)hWnd)
	{
		wchar_t str[256] = { 0, };
		GetWindowText(mhMainWnd, str, 256);
		mWndCaption.assign(str);

		RECT rect;
		GetClientRect((HWND)hWnd, &rect);
		mClientWidth = rect.right - rect.left;
		mClientHeight = rect.bottom - rect.top;

		Initialize();
	}


	//메서드
public:
	void CalculateFrameStatus(GameTimer& timer);	//프레임율 계산
	void Update(GameTimer& timer);	//자료 업데이트
	void Render();	//렌더링

	void PushStaticGeometry(std::string key, std::unique_ptr<D3D12StaticGeometry> geo);	//스태틱 지오메트리 추가
	void PushSkeletalGeometry(std::string key, std::unique_ptr<D3D12SkeletalGeometry> geo);	//스켈레탈 지오메트리 추가
	void PushTexture(std::string key, std::unique_ptr<D3D12Texture> tex);	//텍스쳐 추가
	void PushMaterial(std::string key, std::unique_ptr<D3D12Material> mat);	//메테리얼 추가

	//void SyncResourceManager(ResourceManager& resources);	//리소스 매니저와 자원 동기화
	//void ChangeDefaultShape(ResourceManager& resources, int submeshIdx);	//subMeshIdx의 도형으로 변경. 만약 렌더아이템에 없을 시 렌더아이템도 빌드
	//
	//void CopyToD3D12Resource(ResourceManager& resources);	//리소스 매니저 자원 전환
	//void BuildDefaultShapeFromResources(ResourceManager& resources, std::string shapeName);	//기본 도형 빌드

	/*------------------------------------------------------------------------------------------------------*/
	void OnResize();	//화면크기가 변할 때 처리
	void OnMouseDown(WPARAM btnState, int x, int y);	//마우스 다운
	void OnMouseUp(WPARAM btnState, int x, int y);	//마우스 업
	void OnMouseMove(WPARAM btnState, int x, int y);	//마우스 무브

private:
	bool Initialize();	//초기화
	float AspectRatio() const;	//화면 비율 리턴
	void LogAdapters();	//어댑터 받아오기
	void LogAdapterOutputs(IDXGIAdapter* adapter);	//어댑터 출력
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);	//디스플레이 모드 출력

	void UpdateInstanceData(const GameTimer& timer);	//인스턴싱 데이터 업데이트
	void UpdateMaterialBuffer(const GameTimer& timer);	//메테리얼 버퍼 업데이트
	void UpdateSkinnedCBs(const GameTimer& timer);	//스킨드상수버퍼 업데이트
	void UpdateMainPassCB(const GameTimer& timer);	//메인패스상수버퍼 업데이트
	void UpdateSsaoCB(const GameTimer& gt);	//SSAO상수버퍼 업데이트

	/*------------------------------------------------------------------------------------------------------*/
	ID3D12Resource* CurrentBackBuffer() const;	//현재 백버퍼 리턴
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;	//현재 백버퍼뷰 리턴
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;	//깊이스텐실뷰 리턴

	/*------------------------------------------------------------------------------------------------------*/
	void CreateCommandObjects();	//커맨드 오브젝트 생성
	void CreateSwapChain();	//스왑체인 생성
	void CreateRTVAndDSVDescriptorHeaps();	//RTV, DSV 힙 생성

	void CreateMSAARTVHeaps();	//MSAARTV 힙 생성
	void CreateMSAART();	//MSAARenderTarge 버퍼 생성

	bool LoadTexture(Texture* texture);	//텍스쳐 로드

	void PushTextureDescriptorHeap();
	void BuildD3DTexture(Texture* tex);
	void BuildD3DMaterial(Material* mat);
	void BuildD3DGeometry(Geometry* geo);

	//void BuildStaticRenderItem(ResourceManager& resources, const std::string key, int instanceNum);	//스태틱 렌더아이템 생성
	//void BuildSkeletalRenderItem(ResourceManager& resources, const std::string key, int instanceNum);	//스켈레탈 렌더아이템 생성
	//void BuildDefaultShapeRenderItem(ResourceManager& resources, const std::string key, int instanceNum);	//기본도형 렌더아이템 생성

	void BuildRootSignature();	//루트 시그네쳐 빌드

	void BuildSsaoRootSignature();	//SSAO용 루트 시그네쳐 빌드

	void BuildDescriptorHeaps();	//서술자 힙 빌드
	void BuildShadersAndInputLayout();	//셰이더 입력서술 빌드
	void BuildPSOs();	//PSO빌드
	void BuildFrameResources();	//프레임 리소스 빌드
	void FlushCommandQueue();	//현재 펜스까지 명령 대기

	void DrawNormalsAndDepth();	//노멀과 깊이 그리기
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);	//렌더아이템 그리기

	void ClearResource();	//자원 클리어
	//void ConvertResourceToD3D12(ResourceManager& resources);	//리소스매니저 자원 D3D용으로 전환
	/*------------------------------------------------------------------------------------------------------*/
	int FindTextureIndex(std::string texName);	//텍스쳐 인덱스 찾기

	/*------------------------------------------------------------------------------------------------------*/
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();	//샘플러 불러오기

	//필드
private:
	HWND mhMainWnd;	//윈도우 핸들
	std::wstring mWndCaption;	//윈도우 캡션
	uint32_t mClientWidth = 0; //윈도우 폭
	uint32_t mClientHeight = 0; //윈도우 높이

	POINT mLastMousePos;	//마우스 이전 위치
	float mTheta = -0.5f * DirectX::XM_PI;
	float mPhi = 0.5f * DirectX::XM_PI;
	float mRadius = 10.0f;

	/*------------------------------------------------------------------------------------------------------*/
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;	//D3d Driver Type
	CComPtr<ID3D12Device> md3dDevice;	//D3D Device
	CComPtr<IDXGIFactory4> mdxgiFactory;	//DXGI Factory

	/*------------------------------------------------------------------------------------------------------*/
	CComPtr<ID3D12Fence> mFence;	//Fence : Sync CPU - GPU
	UINT64 mCurrentFence = 0;	//현재 펜스 번호

	/*------------------------------------------------------------------------------------------------------*/
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;	//프레임 리소스 벡터 배열
	FrameResource* mCurrFrameResource = nullptr;	//현재 프레임 리소스
	int mCurrFrameResourceIndex = 0;	//현재 프레임리소스 인덱스

	/*------------------------------------------------------------------------------------------------------*/
	UINT mRtvDescriptorSize = 0;	//RTV 서술자 크기
	UINT mDsvDescriptorSize = 0;	//DSV 서술자 크기
	UINT mCbvSrvUavDescriptorSize = 0;	//CBV, SRV, UAV 서술자 크기

	CComPtr<ID3D12DescriptorHeap> mRtvHeap = nullptr;	//RTV 서술자 힙
	CComPtr<ID3D12DescriptorHeap> mDsvHeap = nullptr;	//DSV 서술자 힙
	CComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;	//CBV, SRV, UAV 서술자 힙

	UINT mSsaoHeapIndexStart = 0;	//SSAO 힙 인덱스 스타트
	UINT mSsaoAmbientMapIndex = 0;	//SSAO AmbientMap 인덱스

	/*------------------------------------------------------------------------------------------------------*/
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;	// 백 버퍼 포맷
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	//깊이 스텐실 포맷

	/*------------------------------------------------------------------------------------------------------*/
	UINT m4xMsaaQuality = 0;	//4xMsaa Quality
	bool m4xMsaaState = false; //초기값 false로 설정 true시 에러

	/*------------------------------------------------------------------------------------------------------*/
	CComPtr<ID3D12CommandQueue> mCommandQueue;	//Command Queue
	CComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;	//CmdList Allocator
	CComPtr<ID3D12GraphicsCommandList> mCommandList;	//Command List.
	

	/*------------------------------------------------------------------------------------------------------*/
	static const UINT mSwapChainBufferCount = 2;	// 더블 버퍼링
	int mCurBackBuffer = 0;	//스왑체인 내에서 현재 후면 버퍼
	CComPtr<IDXGISwapChain> mSwapChain;	//스왑체인
	CComPtr<ID3D12Resource> mSwapChainBuffer[mSwapChainBufferCount];	//스왑체인 버퍼
	CComPtr<ID3D12Resource> mDepthStencilBuffer;	//깊이 스텐실 버퍼

	/*------------------------------------------------------------------------------------------------------*/
	CComPtr<ID3D12Resource> mMSAARenderTarget;	//MSAA지원 렌더타겟

	CComPtr<ID3D12DescriptorHeap> mMSAARtvHeap = nullptr;	//RTV 서술자 힙

	/*------------------------------------------------------------------------------------------------------*/
	CComPtr<ID3D12RootSignature> mRootSignature = nullptr;	//루트 시그네처
	CComPtr<ID3D12RootSignature> mSsaoRootSignature = nullptr;	//SSAO 루트 시그네쳐

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;	//입력 레이아웃
	std::vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;	//스킨 메쉬 입력 레이아웃
	std::unordered_map<std::string, CComPtr<ID3DBlob>> mShaders;	//셰이더 
	std::unordered_map<std::string, CComPtr<ID3D12PipelineState>> mPSOs;	//파이프 상태

	/*------------------------------------------------------------------------------------------------------*/
	D3D12_VIEWPORT mScreenViewport;	//ViewPort
	D3D12_RECT mScissorRect;	//ScissorRect

	/*------------------------------------------------------------------------------------------------------*/
	PassConstants mMainPassCB;	//메인패스상수

	/*------------------------------------------------------------------------------------------------------*/
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;	//모든 렌더아이템
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::SIZE];	//패스별 렌더아이템

	std::unordered_map<std::string, std::unique_ptr<D3D12StaticGeometry>> mStaticGeometries;		//D3D12 스태틱 기하형상 자료
	std::unordered_map<std::string, std::unique_ptr<D3D12SkeletalGeometry>> mSkeletalGeometries;		//D3D12 스켈레탈 기하형상 자료
	std::vector<std::unique_ptr<D3D12Texture>> mTextures;	//D3D12 텍스쳐 자료
	std::vector<std::unique_ptr<D3D12Material>> mMaterials;	//D3D12 메테리얼 자료

	/*------------------------------------------------------------------------------------------------------*/
	Camera mCamera;	//씬의 카메라
	DirectX::BoundingFrustum mCamFrustum;	//카메라의 절두체
	bool mFrustumCullingEnabled = false;	//절두체 선별 활성화 부울값

	/*------------------------------------------------------------------------------------------------------*/
	std::unique_ptr<Ssao> mSsao;	//SSAO - 아직은 지원X

	UINT mInstBufferTotalSize = 100;	//인스턴스 버퍼에 넣을 수 있는 총 갯수
	UINT mMatBufferTotalSize = 30;	//메테리얼 버퍼에 넣을 수 있는 총 갯수
	UINT mSkinnedBufferTotalSize = 30;	//스킨드 버퍼에 넣을 수 있는 총 갯수
	UINT mSrvTotalSize = 20;	//텍스쳐 버퍼에 넣을 수 있는 총 갯수

	bool IsShowBone = true;	//본 렌더링 활성화 부울값

};