#include "Direct3D12Renderer.h"

#include <algorithm>
#include <vector>
#include <string>
#include <Windows.h>

using namespace DirectX;
using namespace std;
using namespace ATL;




void Direct3D12Renderer::CalculateFrameStatus(GameTimer& timer)
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = mWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void Direct3D12Renderer::Update(GameTimer& timer)
{
	auto dt = timer.DeltaTime();

	//������ǥ�� ��ī��Ʈ ��ǥ�� ��ȯ
	float eyeX = mRadius * sinf(mPhi) * cosf(mTheta);
	float eyeZ = mRadius * sinf(mPhi) * sinf(mTheta);
	float eyeY = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(eyeX, eyeY, eyeZ, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	mCamera.LookAt(pos, target, up);

	//ī�޶� �� ��� ������Ʈ
	mCamera.UpdateViewMatrix();

	//���� ������ ���ҽ� �ε��� ����
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//AnimateMaterials(timer);
	UpdateInstanceData(timer);
	UpdateSkinnedCBs(timer);
	UpdateMaterialBuffer(timer);
	UpdateMainPassCB(timer);
	//UpdateSsaoCB(timer);
}

void Direct3D12Renderer::Render()
{
	//���� ������ �ڿ��� �Ҵ��ڸ� ������
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	//����Ҵ���, ��ɸ���Ʈ ����
	ThrowIfFailed(cmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc, nullptr));
	//ThrowIfFailed(mCommandList->Reset(cmdListAlloc, mPSOs["opaque"]));
	//ThrowIfFailed(mCommandList->Reset(cmdListAlloc, mPSOs["skinnedOpaque"]));

	//
	// Normal/depth pass.
	//
	//mCommandList->SetGraphicsRootSignature(mRootSignature);
	//DrawNormalsAndDepth();

	//
	// Compute SSAO.
	// 

	//mCommandList->SetGraphicsRootSignature(mSsaoRootSignature);
	//mSsao->ComputeSsao(mCommandList, mCurrFrameResource, 3);

	
	//
	// Main rendering pass.
	//
	mCommandList->SetGraphicsRootSignature(mRootSignature);

	//����Ʈ �����簢����
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	/**/
	//����� �踮�� ��ȯ ���� -> ����Ÿ��
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//RTV, DSV Ŭ����
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	if (!mAllRitems.empty()) {
		//������ ���̺� (�ؽ���)
		if (mSrvDescriptorHeap != nullptr) {
			ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap };
			mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
			mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		}

		//���̴� �ڿ� ������ (���׸���)
		if (mCurrFrameResource->MaterialBuffer != nullptr) {
			auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
			mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());
		}

		//������ۼ����� (Pass�ڷ�)
		if (mCurrFrameResource->PassCB != nullptr) {
			auto passCB = mCurrFrameResource->PassCB->Resource();
			mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
		}

		//
		//�׸��� ����
		//
		mCommandList->SetPipelineState(mPSOs["opaque"]);
		DrawRenderItems(mCommandList, mRitemLayer[(int)RenderLayer::Opaque]);


		mCommandList->SetPipelineState(mPSOs["skinnedOpaque"]);
		DrawRenderItems(mCommandList, mRitemLayer[(int)RenderLayer::SkinnedOpaque]);
	}

	//Change Barrier
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurBackBuffer = (mCurBackBuffer + 1) % mSwapChainBufferCount;
	
	/*	*/
	//���� ��Ÿ���� �������� ��ɵ��� ��ȣ�ϱ� ���� ��Ÿ�� ������Ų��.
	mCurrFrameResource->Fence = ++mCurrentFence;

	//���� ��Ÿ������ �� ��Ÿ�� �������� �����Ѵ�(Signal���)
	mCommandQueue->Signal(mFence, mCurrentFence);


}

void Direct3D12Renderer::PushStaticGeometry(std::string key, std::unique_ptr<D3D12StaticGeometry> geo)
{
	mStaticGeometries[key] = move(geo);
}

void Direct3D12Renderer::PushSkeletalGeometry(std::string key, std::unique_ptr<D3D12SkeletalGeometry> geo)
{
	mSkeletalGeometries[key] = move(geo);
}

void Direct3D12Renderer::PushTexture(std::string key, std::unique_ptr<D3D12Texture> tex)
{
	mTextures.push_back(move(tex));
}

void Direct3D12Renderer::PushMaterial(std::string key, std::unique_ptr<D3D12Material> mat)
{
	mMaterials.push_back(move(mat));
}

/*
void Direct3D12Renderer::SyncResourceManager(ResourceManager& resources)
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// ��� ��ɵ��� �÷���
	FlushCommandQueue();

	//��� ��� ���� - �����迭�� ���ε� �ϱ� ���� CmdList���
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc, nullptr));

	//
	//Texture
	//
	auto& texs = resources.GetTextures();
	if (texs.size() != mTextures.size()) {
		for (int idx = mTextures.size(); idx < texs.size(); ++idx) {
			BuildD3DTexture(texs[idx].get());
		}
	}

	//
	//Material
	//
	auto& mats = resources.GetMaterials();
	if (mats.size() != mMaterials.size()) {
		for (int idx = mMaterials.size(); idx < mats.size(); ++idx) {
			BuildD3DMaterial(mats[idx].get());
		}
	}

	//
	//StaticGeo
	//
	auto& staticGeos = resources.GetStaticGeometries();
	if (staticGeos.size() != mStaticGeometries.size()) {
		for (int idx = mStaticGeometries.size(); idx < staticGeos.size(); ++idx) {
			BuildD3DGeometry(staticGeos[idx].get());
		}
	}

	//
	//SkeletalGeo
	//
	auto& skeletalGeos = resources.GetSkeletalGeometries();
	if (skeletalGeos.size() != mSkeletalGeometries.size()) {
		for (int idx = mSkeletalGeometries.size(); idx < skeletalGeos.size(); ++idx) {
			BuildD3DGeometry(skeletalGeos[idx].get());
		}
	}


	//��� ����
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//�ڿ� ����� ��� ���� ó��
	FlushCommandQueue();
}

void Direct3D12Renderer::ChangeDefaultShape(ResourceManager& resources, int submeshIdx)
{
	RenderItem* targetGeo = nullptr;
	//���������ۿ��� ã��
	for (auto& e : mAllRitems) {
		if (e->Geo->GeoData->Name.compare("DefaultShape") == 0) {
			targetGeo = e.get();
		}
	}

	//���������ۿ��� �� ã���� �ÿ�
	if (targetGeo == nullptr) {
		//���ҽ��Ŵ������� D3D�ڿ� ����
		SyncResourceManager(resources);

		//D3D�ڿ����� �⺻���� ���������� ����
		BuildDefaultShapeRenderItem(resources, "Box", 1);

		targetGeo = mAllRitems[mAllRitems.size() - 1].get();
	}


	//�������ۿ����� ��ġ�� �ε��� ����, ��ġ ����
	auto& targetSubmesh = mStaticGeometries["DefaultShape"]->GeoData->SubGeoInfos[submeshIdx];
	targetGeo->Name = targetSubmesh.Name;
	targetGeo->BaseVertexLocation = targetSubmesh.BaseVertexLocation;
	targetGeo->IndexCount = targetSubmesh.IndexCount;
	targetGeo->StartIndexLocation = targetSubmesh.StartIndexLocation;

	//ī�޶� ����
	mTheta = -0.5f * DirectX::XM_PI;
	mPhi = 0.5f * DirectX::XM_PI;
	mRadius = 10.0f;
}

void Direct3D12Renderer::CopyToD3D12Resource(ResourceManager& resources)
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// ��� ��ɵ��� �÷���
	FlushCommandQueue();

	//��� ��� ����
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc, nullptr));

	//�ڿ� ��ȯ 
	ConvertResourceToD3D12(resources);

	//�ڿ� ����
	//�ؽ��ĸ� ���� �� ����
	BuildDescriptorHeaps();

	//���������� ����
	for (auto& e : mStaticGeometries) {
		BuildStaticRenderItem(resources, e.first, 1);
	}

	for (auto& e : mSkeletalGeometries) {
		BuildSkeletalRenderItem(resources, e.first, 1);
	}

	//ī�޶� ����
	mTheta = -0.5f * DirectX::XM_PI;
	mPhi = 0.5f * DirectX::XM_PI;
	mRadius = 10.0f;

	//��� ����
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//�ڿ� ����� ��� ���� ó��
	FlushCommandQueue();

}
void Direct3D12Renderer::BuildDefaultShapeFromResources(ResourceManager& resources, std::string shapeName)
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// ��� ��ɵ��� �÷���
	FlushCommandQueue();


	//��� ��� ���� - �����迭�� ���ε� �ϱ� ���� CmdList���
	ThrowIfFailed(mDirectCmdListAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc, nullptr));


	//�ڿ� ��ȯ 
	ConvertResourceToD3D12(resources);

	//�⺻���� ���������� ����
	BuildDefaultShapeRenderItem(resources, shapeName, 1);

	//ī�޶� ����
	mTheta = -0.5f * DirectX::XM_PI;
	mPhi = 0.5f * DirectX::XM_PI;
	mRadius = 10.0f;


	//��� ����
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//�ڿ� ����� ��� ���� ó��
	FlushCommandQueue();
	
	
}
*/
void Direct3D12Renderer::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// ��� ��ɵ��� �÷���
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc, nullptr));


	// ����ü�ΰ� ���̹��� �� ����
	for (int i = 0; i < mSwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Detach();
	mDepthStencilBuffer.Detach();

	// ����ü�� ��������
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		mSwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < mSwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i], nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// ���� ���ٽ� ���� ����
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(&mDepthStencilBuffer)));

	// ���� ���ٽ� �� �Ӽ�
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, &dsvDesc, DepthStencilView());

	// �踮�� ��ȯ �Ϲ� > ���̾��� ����
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));


	// �������� ��� ����
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �������� ó��
	FlushCommandQueue();

	//����Ʈ �۾����� ũ�� ������Ʈ
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, (LONG)mClientWidth, (LONG)mClientHeight };

	//ī�޶� ����
	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	BoundingFrustum::CreateFromMatrix(mCamFrustum, mCamera.GetProj());

	//SSAO ����
	if (mSsao != nullptr)
	{
		mSsao->OnResize(mClientWidth, mClientHeight);

		// �ڿ��� ����Ǹ�, �������� ���尡 �ʿ�
		mSsao->RebuildDescriptors(mDepthStencilBuffer);
	}
}

void Direct3D12Renderer::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Direct3D12Renderer::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Direct3D12Renderer::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		//���콺 �� �ȼ� �̵��� 1/4���� ����
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		//mPhi���� ����
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		//���콺 �� �ȼ��� ����� 0.005������ ����
		float dx = 0.3f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.3f * static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		//�������� ����
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 200.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

bool Direct3D12Renderer::Initialize()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// D3D12 ����� ���� Ȱ��ȭ
	{
		CComPtr<ID3D12Debug> debugController;

		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

		//debugController->EnableDebugLayer();
	}
#endif
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

#ifdef _DEBUG
	LogAdapters();
#endif

	CComPtr<IDXGIAdapter> adapter = nullptr;
	mdxgiFactory->EnumAdapters(0, &adapter);

	// ����̽� ���� �õ�
	HRESULT hardwareResult = D3D12CreateDevice(
		adapter,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// WARP����̽� Fall Back
	if (FAILED(hardwareResult))
	{
		CComPtr<IDXGIAdapter> pWarpAdapter;

		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	//�潺 ����
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	//MultiSample �� ����
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");


	//Ŀ�ǵ� ������Ʈ, ����ü��, RTV/DSV�������� ����
	CreateCommandObjects();
	CreateSwapChain();
	CreateRTVAndDSVDescriptorHeaps();

	//MSAA�� ���� ����
	//CreateMSAARTVHeaps();
	//CreateMSAART();

	// Ŀ�ǵ� ����Ʈ ����
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc, nullptr));

	//�ʱ�ȭ ��� ����
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ʱ�ȭ�� �Ϸ�� ������ ���
	FlushCommandQueue();


	//ī�޶� ����
	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	BoundingFrustum::CreateFromMatrix(mCamFrustum, mCamera.GetProj());

	//SSAO ����
	mSsao = std::make_unique<Ssao>(
		md3dDevice,
		mCommandList,
		mClientWidth, mClientHeight);

	//Build
	BuildRootSignature();
	BuildSsaoRootSignature();

	BuildShadersAndInputLayout();
	BuildPSOs();

	//������ �� ����(�ڿ� �� ����[�ؽ���])
	BuildDescriptorHeaps();

	//�����Ӹ��ҽ� ���(���������� ��(�ν��ͽ����� �ν��ͽ̵����� ��), ���׸��� ��)
	BuildFrameResources();

	mSsao->SetPSOs(mPSOs["ssao"], mPSOs["ssaoBlur"]);

	//OnResize
	OnResize();

	return true;
}


float Direct3D12Renderer::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

void Direct3D12Renderer::LogAdapters() {
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void Direct3D12Renderer::LogAdapterOutputs(IDXGIAdapter* adapter) {
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void Direct3D12Renderer::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format) {
	UINT count = 0;
	UINT flags = 0;

	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void Direct3D12Renderer::UpdateInstanceData(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	auto currInstanceBuffer = mCurrFrameResource->InstanceBuffer.get();
	int instanceCount = 0;

	//�ν��Ͻ� ������ ������Ʈ
	for (auto& e : mAllRitems)
	{
		const auto& instanceData = e->Instances;	//�ν��Ͻ� �����͸� ���� �ε�
		e->InstanceSrvIndex = instanceCount;

		int visibleInstanceCount = 0;
		for (UINT i = 0; i < (UINT)instanceData.size(); ++i)
		{
			XMMATRIX world = XMLoadFloat4x4(&instanceData[i].World);
			XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].TexTransform);

			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

			// View �������� object�� local �������� ��ȯ���
			XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

			// view�������� object�� local �������� ī�޶� ����ü ��ȯ
			BoundingFrustum localSpaceFrustum;
			mCamFrustum.Transform(localSpaceFrustum, viewToLocal);

			// local �������� box/frustum ���� �׽�Ʈ
			if ((localSpaceFrustum.Contains(e->Bounds) != DirectX::DISJOINT) || (mFrustumCullingEnabled == false))
			{
				InstanceData data;
				XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));

				auto curGeoData = e->Geo->GeoData;

				for (auto sub : curGeoData->SubGeoInfos) {
					if (e->Name.compare(sub.Name) == 0) {
						int matIdx = -1;
						for (int j = 0; j < (int)mMaterials.size(); ++j) {
							if (mMaterials[j]->MatData->Name.compare(sub.MatName) == 0) {
								matIdx = j;
								break;
							}
						}
						data.MaterialIndex = matIdx;
						break;
					}
				}
				
				currInstanceBuffer->CopyData(instanceCount++, data);
				++visibleInstanceCount;
			}
		}

	}
}

void Direct3D12Renderer::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		D3D12Material* mat = e.get();

		// ��ȭ���� ���� ������Ʈ�� �Ѵ�
		if (mat->MatData->isDirty) {
			mat->NumFramesDirty = gNumFrameResources;
			mat->MatData->isDirty = false;
		}

		if (mat->NumFramesDirty > 0) {
			MaterialData matData;
			matData.DiffuseAlbedo = mat->MatData->DiffuseAlbedo;
			matData.FresnelR0 = mat->MatData->FresnelR0;
			matData.Roughness = mat->MatData->Roughness;
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatData->MatTransform);
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));

			
			matData.DiffuseMapIndex = FindTextureIndex(mat->MatData->TexName[(int)TextureType::DIFFUSE]);
			matData.NormalMapIndex = FindTextureIndex(mat->MatData->TexName[(int)TextureType::NORMAL]);
			matData.SpecularMapIndex = FindTextureIndex(mat->MatData->TexName[(int)TextureType::SPECULAR]);

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);
			mat->NumFramesDirty--;
		}

	}
}

void Direct3D12Renderer::UpdateSkinnedCBs(const GameTimer& timer)
{
	auto currSkinnedCB = mCurrFrameResource->SkinnedCB.get();
	for (int i = 0; i < (int)mRitemLayer[(int)RenderLayer::SkinnedOpaque].size(); ++i) {

		auto CurRitem = mRitemLayer[(int)RenderLayer::SkinnedOpaque][i];

		if (CurRitem->SkinnedModelInst == nullptr)
			return;

		CurRitem->SkinnedModelInst->UpdateSkinnedAnimation(timer.DeltaTime());

		SkinnedConstants skinnedConstants;
		std::copy(
			std::begin(CurRitem->SkinnedModelInst->FinalTransforms),
			std::end(CurRitem->SkinnedModelInst->FinalTransforms),
			&skinnedConstants.BoneTransforms[0]);

		currSkinnedCB->CopyData(i, skinnedConstants);
	}
}

void Direct3D12Renderer::UpdateMainPassCB(const GameTimer& timer)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = mCamera.GetNearZ();
	mMainPassCB.FarZ = mCamera.GetFarZ();
	mMainPassCB.TotalTime = timer.TotalTime();
	mMainPassCB.DeltaTime = timer.DeltaTime();
	mMainPassCB.AmbientLight = { 0.2f, 0.2f, 0.2f, 0.8f };

	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.7f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Direct3D12Renderer::UpdateSsaoCB(const GameTimer& gt)
{
	SsaoConstants ssaoCB;

	XMMATRIX P = mCamera.GetProj();

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	ssaoCB.Proj = mMainPassCB.Proj;
	ssaoCB.InvProj = mMainPassCB.InvProj;
	XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(P * T));

	mSsao->GetOffsetVectors(ssaoCB.OffsetVectors);

	auto blurWeights = mSsao->CalcGaussWeights(2.5f);
	ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

	ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / mSsao->SsaoMapWidth(), 1.0f / mSsao->SsaoMapHeight());

	// Coordinates given in view space.
	ssaoCB.OcclusionRadius = 0.5f;
	ssaoCB.OcclusionFadeStart = 0.2f;
	ssaoCB.OcclusionFadeEnd = 1.0f;
	ssaoCB.SurfaceEpsilon = 0.05f;

	auto currSsaoCB = mCurrFrameResource->SsaoCB.get();
	currSsaoCB->CopyData(0, ssaoCB);
}

ID3D12Resource* Direct3D12Renderer::CurrentBackBuffer() const
{
	assert(mSwapChainBuffer);
	return mSwapChainBuffer[mCurBackBuffer];
}

D3D12_CPU_DESCRIPTOR_HANDLE Direct3D12Renderer::CurrentBackBufferView() const
{
	assert(mRtvHeap);
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE Direct3D12Renderer::DepthStencilView() const
{
	assert(mDsvHeap);
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void Direct3D12Renderer::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&mDirectCmdListAlloc)));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc, // ������ cmdAlloc
		nullptr,                   // �ʱ�ȭ PSO
		IID_PPV_ARGS(&mCommandList)));

	mCommandList->Close();
}

void Direct3D12Renderer::CreateSwapChain() {
	mSwapChain.Detach();

	/*	*/
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = mSwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue,
		&sd,
		&mSwapChain));

}

void Direct3D12Renderer::CreateRTVAndDSVDescriptorHeaps() {
	// RTV ���� = ����ü�ο� 2, ȭ�� ��ָ����� +1, �ֺ��� ������ +2.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = mSwapChainBufferCount + 3;	//����ü�� �� ����
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;	//���� ���ٽ� �� ���� �׸��� ������ +1
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

void Direct3D12Renderer::CreateMSAARTVHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = 1;	//MSAA ����Ÿ�� ����
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(&mMSAARtvHeap)));
}

void Direct3D12Renderer::CreateMSAART()
{
	// MSAA RT ����
	D3D12_RESOURCE_DESC msaaBufferDesc;
	msaaBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	msaaBufferDesc.Alignment = 0;
	msaaBufferDesc.Width = mClientWidth;
	msaaBufferDesc.Height = mClientHeight;
	msaaBufferDesc.DepthOrArraySize = 1;
	msaaBufferDesc.MipLevels = 1;
	msaaBufferDesc.Format = mBackBufferFormat;
	msaaBufferDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	msaaBufferDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	msaaBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	msaaBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&msaaBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mMSAARenderTarget)));

	// MSAA RTV �Ӽ�
	D3D12_RENDER_TARGET_VIEW_DESC msaaRTVDesc;
	
	msaaRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
	msaaRTVDesc.Format = mBackBufferFormat;

	md3dDevice->CreateRenderTargetView(mMSAARenderTarget, &msaaRTVDesc, mMSAARtvHeap->GetCPUDescriptorHandleForHeapStart());

	/**/
	// �踮�� ��ȯ �Ϲ� > ���� ����
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMSAARenderTarget,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));
	
}


inline bool exists_test3(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

bool Direct3D12Renderer::LoadTexture(Texture* texture)
{
	auto tempTex = std::make_unique<D3D12Texture>();
	tempTex->TexData = texture;

	auto& fileName = tempTex->TexData->Name;

	//���ڿ� �� ���� "."�� ��ġ �˻� �� Ȯ���ڸ�� �޾ƿ���
	auto dotIdx = fileName.find_last_of(".");
	std::string ext = fileName.substr(dotIdx, fileName.size() - dotIdx);

	if (ext.compare(".dds") == 0) {
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice,
			mCommandList, StringToWString(fileName).c_str(),
			tempTex->Resource, tempTex->UploadHeap));
	}
	else {
		std::unique_ptr<uint8_t[]> decodedData;
		D3D12_SUBRESOURCE_DATA subresource;

		LoadWICTextureFromFile(md3dDevice, StringToWString(tempTex->TexData->Name).c_str(), &tempTex->Resource, decodedData, subresource);

		if (tempTex->Resource == nullptr) {	//�ؽ��� �б� ����
			return false;
		}

		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(tempTex->Resource, 0, 1);

		ThrowIfFailed(md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&tempTex->UploadHeap)));


		UpdateSubresources(mCommandList, tempTex->Resource, tempTex->UploadHeap,
			0, 0, 1, &subresource);

		mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(tempTex->Resource,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}

	mTextures.push_back(move(tempTex));

	return true;
}

void Direct3D12Renderer::PushTextureDescriptorHeap()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	hDescriptor.Offset((int)mTextures.size() - 1, mCbvSrvUavDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.MipLevels = mTextures[mTextures.size() - 1]->Resource->GetDesc().MipLevels;
	srvDesc.Format = mTextures[mTextures.size() - 1]->Resource->GetDesc().Format;

	md3dDevice->CreateShaderResourceView(mTextures[mTextures.size() - 1]->Resource, &srvDesc, hDescriptor);
}

void Direct3D12Renderer::BuildD3DTexture(Texture* tex)
{
	if (mTextures.size() < mSrvTotalSize) {
		LoadTexture(tex);	//������ �о mTexture�� ����
		PushTextureDescriptorHeap();	//���� ���� �ؽ��ĸ� ��ũ���� ���� �߰�.
	}
	else {
		
	}

}

void Direct3D12Renderer::BuildD3DMaterial(Material* mat)
{
	//���׸���
	auto tempMat = make_unique<D3D12Material>();
	tempMat->MatData = mat;
	tempMat->MatCBIndex = (int)mMaterials.size();
	tempMat->NumFramesDirty = gNumFrameResources;

	for (int i = 0; i < (int)mTextures.size(); ++i) {	//Diffuse�ؽ��� �˻�
		if (mTextures[i]->TexData->Name.compare(tempMat->MatData->TexName[(int)TextureType::DIFFUSE]) == 0) {
			tempMat->DiffuseSrvHeapIndex = i;
			break;
		}
	}

	for (int i = 0; i < (int)mTextures.size(); ++i) {	//Normal�ؽ��� �˻�
		if (mTextures[i]->TexData->Name.compare(tempMat->MatData->TexName[(int)TextureType::NORMAL]) == 0) {
			tempMat->NormalSrvHeapIndex = i;
			break;
		}
	}

	for (int i = 0; i < (int)mTextures.size(); ++i) {	//Specular�ؽ��� �˻�
		if (mTextures[i]->TexData->Name.compare(tempMat->MatData->TexName[(int)TextureType::SPECULAR]) == 0) {
			tempMat->SpecularSrvHeapIndex = i;
			break;
		}
	}

	mMaterials.push_back(move(tempMat));	//Mat D3D�� ����




	
}

void Direct3D12Renderer::BuildD3DGeometry(Geometry* geo)
{
	//StaticGeo
	if (geo->IsStatic) {
		auto temp = make_unique<D3D12StaticGeometry>();
		temp->GeoData = geo;
		auto staticGeo = (StaticGeometry*)temp->GeoData;

		//���ҽ� �Ŵ����� ������Ʈ������
		//D3D12������Ʈ���� ��ȯ�Ϸ��� GPU�� ���ε��� �����͵� �����ؾ��Ѵ�.
		//���� GPU�� ���� �� �ε��� ���ε� 
		//
		auto& vertices = staticGeo->Mesh.Vertices;
		auto& indices = staticGeo->Mesh.GetIndices16();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		//�������� CPU
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &temp->VertexBufferCPU));
		CopyMemory(temp->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		//�ε������� CPU
		ThrowIfFailed(D3DCreateBlob(ibByteSize, &temp->IndexBufferCPU));
		CopyMemory(temp->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		//�������� GPU
		temp->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice,
			mCommandList, vertices.data(), vbByteSize, temp->VertexBufferUploader);

		//�ε������� GPU
		temp->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice,
			mCommandList, indices.data(), ibByteSize, temp->IndexBufferUploader);


		//��������, �ε��� ���� ����
		temp->VertexByteStride = sizeof(Vertex);	//�������� ���� ������
		temp->VertexBufferByteSize = vbByteSize;	//�������� �� ������
		temp->IndexFormat = DXGI_FORMAT_R16_UINT;	//�ε������� ����
		temp->IndexBufferByteSize = ibByteSize;	//�ε������� �� ������


		//���� �� �ε��� �迭 �ʱ�ȭ( �̹� ���������Ƿ� )
		/*
		vertices.clear();
		indices.clear();
		temp->DisposeUploaders();
		*/
		//����� �ʿ� ����
		mStaticGeometries[temp->GeoData->Name] = move(temp);
	}

	//SkeletalGeo
	else {
		auto temp = make_unique<D3D12SkeletalGeometry>();
		temp->GeoData = geo;
		auto skeletalGeo = (SkeletalGeometry*)temp->GeoData;

		//���ҽ� �Ŵ����� ������Ʈ������
		//D3D12������Ʈ���� ��ȯ�Ϸ��� GPU�� ���ε��� �����͵� �����ؾ��Ѵ�.
		//���� GPU�� ���� �� �ε��� ���ε� 
		//
		auto& vertices = skeletalGeo->Mesh.Vertices;
		auto& indices = skeletalGeo->Mesh.GetIndices16();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(SkinnedVertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		//�������� CPU
		ThrowIfFailed(D3DCreateBlob(vbByteSize, &temp->VertexBufferCPU));
		CopyMemory(temp->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		//�ε������� CPU
		ThrowIfFailed(D3DCreateBlob(ibByteSize, &temp->IndexBufferCPU));
		CopyMemory(temp->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		//�������� GPU
		temp->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice,
			mCommandList, vertices.data(), vbByteSize, temp->VertexBufferUploader);

		//�ε������� GPU
		temp->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(md3dDevice,
			mCommandList, indices.data(), ibByteSize, temp->IndexBufferUploader);

		//��������, �ε��� ���� ����
		temp->VertexByteStride = sizeof(SkinnedVertex);	//�������� ���� ������
		temp->VertexBufferByteSize = vbByteSize;	//�������� �� ������
		temp->IndexFormat = DXGI_FORMAT_R16_UINT;	//�ε������� ����
		temp->IndexBufferByteSize = ibByteSize;	//�ε������� �� ������

		//���� �� �ε��� �迭 �ʱ�ȭ( �̹� ���������Ƿ� )
		/*
		vertices.clear();
		indices.clear();
		temp->DisposeUploaders();
		*/
		//����� �ʿ� ����
		mSkeletalGeometries[temp->GeoData->Name] = move(temp);
	}
}

/*
void Direct3D12Renderer::BuildStaticRenderItem(ResourceManager& resources, const std::string key, int instanceNum)
{
	const auto& subInfo = mStaticGeometries[key]->GeoData->SubGeoInfos;
	for (int subMeshIdx = 0; subMeshIdx < subInfo.size(); ++subMeshIdx) {
		auto rItem = make_unique<RenderItem>();
		rItem->Name = subInfo[subMeshIdx].Name;
		rItem->Geo = mStaticGeometries[key].get();
		rItem->IndexCount = subInfo[subMeshIdx].IndexCount;
		rItem->StartIndexLocation = subInfo[subMeshIdx].StartIndexLocation;
		rItem->BaseVertexLocation = subInfo[subMeshIdx].BaseVertexLocation;

		//�ν��Ͻ� �� ����
		UINT instanceSrvIdx = 0;
		for (int i = 0; i < mAllRitems.size(); ++i) {
			instanceSrvIdx += (UINT)mAllRitems[i]->Instances.size();
		}

		rItem->InstanceSrvIndex = instanceSrvIdx;
		rItem->Instances.resize(instanceNum);

		for (int instIdx = 0; instIdx < instanceNum; ++instIdx) {

			XMMATRIX pos = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
			XMMATRIX scale = XMMatrixScaling(0.02f, 0.02f, 0.02f);
			XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(0.0f), XMConvertToRadians(0.0f));

			XMStoreFloat4x4(&rItem->Instances[instIdx].World, rotation * scale * pos);

			//rItem->Instances[instIdx].World = MathHelper::Identity4x4();
			rItem->Instances[instIdx].TexTransform = MathHelper::Identity4x4();

			//Material �˻�
			UINT matIdx = 0;
			for (int j = 0; j < mMaterials.size(); ++j) {
				auto& curName = mMaterials[j]->MatData->Name;

				if (curName.compare(subInfo[subMeshIdx].MatName) == 0) {
					matIdx = j;
					break;
				}
			}
			rItem->Instances[instIdx].MaterialIndex = matIdx;
		}

		mRitemLayer[(int)RenderLayer::Opaque].push_back(rItem.get());
		mAllRitems.push_back(move(rItem));
	}

}

void Direct3D12Renderer::BuildSkeletalRenderItem(ResourceManager& resources, const std::string key, int instanceNum)
{
	const auto& subInfo = mSkeletalGeometries[key]->GeoData->SubGeoInfos;
	bool IsSkinned = false;
	for (int subMeshIdx = 0; subMeshIdx < subInfo.size(); ++subMeshIdx) {
		auto rItem = make_unique<RenderItem>();
		rItem->Name = subInfo[subMeshIdx].Name;
		rItem->Geo = mSkeletalGeometries[key].get();
		rItem->IndexCount = subInfo[subMeshIdx].IndexCount;
		rItem->StartIndexLocation = subInfo[subMeshIdx].StartIndexLocation;
		rItem->BaseVertexLocation = subInfo[subMeshIdx].BaseVertexLocation;

		//�ν��Ͻ� �� ����
		UINT instanceSrvIdx = 0;
		for (int i = 0; i < mAllRitems.size(); ++i) {
			instanceSrvIdx += (UINT)mAllRitems[i]->Instances.size();
		}

		rItem->InstanceSrvIndex = instanceSrvIdx;
		rItem->Instances.resize(instanceNum);
		
		for (int instIdx = 0; instIdx < instanceNum; ++instIdx) {

			XMMATRIX pos = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
			XMMATRIX scale = XMMatrixScaling(0.02f, 0.02f, 0.02f);
			XMMATRIX rotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(0.0f), XMConvertToRadians(0.0f));

			XMStoreFloat4x4(&rItem->Instances[instIdx].World, rotation * scale * pos);

			//rItem->Instances[i].World = MathHelper::Identity4x4();
			rItem->Instances[instIdx].TexTransform = MathHelper::Identity4x4();

			//Material �˻�
			UINT matIdx = 0;
			for (int j = 0; j < mMaterials.size(); ++j) {
				auto& curName = mMaterials[j]->MatData->Name;

				if (curName.compare(subInfo[subMeshIdx].MatName) == 0) {
					matIdx = j;
					break;
				}
			}
			rItem->Instances[instIdx].MaterialIndex = matIdx;
		}


		//�ִϸ��̼� ����
		auto skeletalGeo = (SkeletalGeometry*)rItem->Geo->GeoData;
		if (skeletalGeo->Skeleton->IsAnimationable && resources.GetAnimations().get()->find("mixamo.com") != resources.GetAnimations().get()->end()) {	//�ִϸ��̼� ������ ������ ���
			auto clip = *resources.GetAnimations().get();

			SkinnedData* skinData = new SkinnedData();
			skinData->Set(clip["mixamo.com"].Skeleton, clip);

			D3D12SkinnedModelInstance* skinModel = new D3D12SkinnedModelInstance();
			skinModel->SkinnedInfo = skinData;
			skinModel->TimePos = 0.0f;
			skinModel->SkinnedInfo = skinData;
			skinModel->ClipName = "mixamo.com";
			skinModel->FinalTransforms.resize(clip["mixamo.com"].Skeleton->mJoints.size());
			rItem->SkinnedModelInst = skinModel;	//�ִϸ��̼� ����
			rItem->SkinnedCBIndex = 0;	//���ҽ� �� ���� Bone ��� �ε���

			mRitemLayer[(int)RenderLayer::SkinnedOpaque].push_back(rItem.get());
		}
		else {
			mRitemLayer[(int)RenderLayer::Opaque].push_back(rItem.get());
		}

		mAllRitems.push_back(move(rItem));
	}
}

void Direct3D12Renderer::BuildDefaultShapeRenderItem(ResourceManager& resources, const std::string key, int instanceNum)
{
	const auto& subInfo = mStaticGeometries["DefaultShape"]->GeoData->SubGeoInfos;

	for (int subMeshIdx = 0; subMeshIdx < subInfo.size(); ++subMeshIdx) {
		if (subInfo[subMeshIdx].Name.compare(key) != 0)
			continue;

		auto rItem = make_unique<RenderItem>();
		rItem->Name = subInfo[subMeshIdx].Name;
		rItem->Geo = mStaticGeometries["DefaultShape"].get();
		rItem->IndexCount = subInfo[subMeshIdx].IndexCount;
		rItem->StartIndexLocation = subInfo[subMeshIdx].StartIndexLocation;
		rItem->BaseVertexLocation = subInfo[subMeshIdx].BaseVertexLocation;

		//�ν��Ͻ� �� ����
		UINT instanceSrvIdx = 0;
		for (int i = 0; i < mAllRitems.size(); ++i) {
			instanceSrvIdx += (UINT)mAllRitems[i]->Instances.size();
		}

		rItem->InstanceSrvIndex = instanceSrvIdx;
		rItem->Instances.resize(instanceNum);

		for (int instIdx = 0; instIdx < instanceNum; ++instIdx) {
			rItem->Instances[instIdx].World = MathHelper::Identity4x4();
			rItem->Instances[instIdx].TexTransform = MathHelper::Identity4x4();

			//Material �˻�
			UINT matIdx = 0;
			for (int j = 0; j < mMaterials.size(); ++j) {
				auto& curName = mMaterials[j]->MatData->Name;

				if (curName.compare(subInfo[subMeshIdx].MatName) == 0) {
					matIdx = j;
					break;
				}
			}
			rItem->Instances[instIdx].MaterialIndex = matIdx;
		}


		mRitemLayer[(int)RenderLayer::Opaque].push_back(rItem.get());
		mAllRitems.push_back(move(rItem));

		break;
	}

}
*/
void Direct3D12Renderer::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mSrvTotalSize, 0, 0); //tex

	// ��Ʈ �Ű������� ���̺�, ������, ��Ʈ ����� �����ȴ�
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	//�����ս� TIP: ���� ���� ���Ǵ� �� ���� ���� ���� ���Ǵ� ������ ����.
	slotRootParameter[0].InitAsShaderResourceView(0, 1);	//InstanceData Descriptor
	slotRootParameter[1].InitAsShaderResourceView(1, 1);	//MetarialData Descriptor
	slotRootParameter[2].InitAsConstantBufferView(0);		//PassCB Descriptor
	slotRootParameter[3].InitAsConstantBufferView(1);		//SkinnedData Descriptor
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); //TextureMaps

	auto staticSamplers = GetStaticSamplers();

	//��Ʈ �ñ״��Ĵ� ��Ʈ �Ű������� �迭�̴�
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//�̱� ���� ��Ʈ �ñ״�ó ����
	CComPtr<ID3DBlob> serializedRootSig = nullptr;
	CComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void Direct3D12Renderer::BuildSsaoRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstants(1, 1);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	CComPtr<ID3DBlob> serializedRootSig = nullptr;
	CComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mSsaoRootSignature)));
}


void Direct3D12Renderer::BuildDescriptorHeaps()
{
	//
	// SRV �� ����
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = mSrvTotalSize;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	if (mTextures.size() > 0) {
		//�ؽ��� �ڿ��� ����
		for (int i = 0; i < (int)mTextures.size(); ++i) {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			srvDesc.Texture2D.MipLevels = mTextures[i]->Resource->GetDesc().MipLevels;
			srvDesc.Format = mTextures[i]->Resource->GetDesc().Format;

			md3dDevice->CreateShaderResourceView(mTextures[i]->Resource, &srvDesc, hDescriptor);

			// next descriptor
			hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		}
	}

	mSsaoHeapIndexStart = 10;
	mSsaoAmbientMapIndex = mSsaoHeapIndexStart + 3;

	//SSAO �ڿ� ������ ����
	mSsao->BuildDescriptors(
		mDepthStencilBuffer,
		GetCpuSrv(mSsaoHeapIndexStart),
		GetGpuSrv(mSsaoHeapIndexStart),
		GetRtv(mSwapChainBufferCount),
		mCbvSrvUavDescriptorSize,
		mRtvDescriptorSize);

}

void Direct3D12Renderer::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO skinnedDefines[] =
	{
		"SKINNED", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skinnedVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["opaquePS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["drawNormalsVS"] = D3DUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["drawNormalsPS"] = D3DUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoVS"] = D3DUtil::CompileShader(L"Shaders\\SSAO.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoPS"] = D3DUtil::CompileShader(L"Shaders\\SSAO.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoBlurVS"] = D3DUtil::CompileShader(L"Shaders\\SSAOBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoBlurPS"] = D3DUtil::CompileShader(L"Shaders\\SSAOBlur.hlsl", nullptr, "PS", "ps_5_1");


	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mSkinnedInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 68, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void Direct3D12Renderer::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;


	ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	basePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	basePsoDesc.pRootSignature = mRootSignature;
	basePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	basePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	basePsoDesc.SampleMask = UINT_MAX;
	basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basePsoDesc.NumRenderTargets = 1;
	basePsoDesc.RTVFormats[0] = mBackBufferFormat;
	basePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	basePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	basePsoDesc.DSVFormat = mDepthStencilFormat;


	//
	// PSO for opaque objects.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = basePsoDesc;
	//opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	//opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	//
	// PSO for skinned pass.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedOpaquePsoDesc = basePsoDesc;
	skinnedOpaquePsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
	skinnedOpaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skinnedVS"]->GetBufferPointer()),
		mShaders["skinnedVS"]->GetBufferSize()
	};
	skinnedOpaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skinnedOpaquePsoDesc, IID_PPV_ARGS(&mPSOs["skinnedOpaque"])));


	//
	// PSO for drawing normals.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPsoDesc = basePsoDesc;
	drawNormalsPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawNormalsVS"]->GetBufferPointer()),
		mShaders["drawNormalsVS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawNormalsPS"]->GetBufferPointer()),
		mShaders["drawNormalsPS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.RTVFormats[0] = Ssao::NormalMapFormat;
	drawNormalsPsoDesc.SampleDesc.Count = 1;
	drawNormalsPsoDesc.SampleDesc.Quality = 0;
	drawNormalsPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawNormalsPsoDesc, IID_PPV_ARGS(&mPSOs["drawNormals"])));

	//
	// PSO for SSAO.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = basePsoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mSsaoRootSignature;
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoVS"]->GetBufferPointer()),
		mShaders["ssaoVS"]->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoPS"]->GetBufferPointer()),
		mShaders["ssaoPS"]->GetBufferSize()
	};

	// SSAO effect does not need the depth buffer.
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mPSOs["ssao"])));

	//
	// PSO for SSAO blur.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurVS"]->GetBufferPointer()),
		mShaders["ssaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurPS"]->GetBufferPointer()),
		mShaders["ssaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&mPSOs["ssaoBlur"])));

}

void Direct3D12Renderer::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice,
			1, mInstBufferTotalSize, mMatBufferTotalSize, mSkinnedBufferTotalSize));
	}
}


void Direct3D12Renderer::FlushCommandQueue() {
	mCurrentFence++;

	ThrowIfFailed(mCommandQueue->Signal(mFence, mCurrentFence));

	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		assert(eventHandle != 0);

		WaitForSingleObject(eventHandle, INFINITE);

		CloseHandle(eventHandle);
	}
}

void Direct3D12Renderer::DrawNormalsAndDepth()
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto normalMap = mSsao->NormalMap();
	auto normalMapRtv = mSsao->NormalMapRtv();

	// Change to RENDER_TARGET.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());


	//������ ���̺� (�ؽ���)
	if (mSrvDescriptorHeap != nullptr) {
		ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		mCommandList->SetGraphicsRootDescriptorTable(4, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}

	//���̴� �ڿ� ������ (���׸���)
	if (mCurrFrameResource->MaterialBuffer != nullptr) {
		auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
		mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());
	}

	//������ۼ����� (Pass�ڷ�)
	if (mCurrFrameResource->PassCB != nullptr) {
		auto passCB = mCurrFrameResource->PassCB->Resource();
		mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	}

	mCommandList->SetPipelineState(mPSOs["drawNormals"]);

	DrawRenderItems(mCommandList, mRitemLayer[(int)RenderLayer::Opaque]);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Direct3D12Renderer::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	int size = sizeof(InstanceData);

	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];
		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		auto instanceBuffer = mCurrFrameResource->InstanceBuffer->Resource();
		cmdList->SetGraphicsRootShaderResourceView(0, instanceBuffer->GetGPUVirtualAddress() + (ri->InstanceSrvIndex * size));

		if (ri->SkinnedModelInst != nullptr)
		{
			auto skinnedCB = mCurrFrameResource->SkinnedCB->Resource();
			UINT skinnedCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(SkinnedConstants));

			D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = skinnedCB->GetGPUVirtualAddress() + ri->SkinnedCBIndex * skinnedCBByteSize;
			cmdList->SetGraphicsRootConstantBufferView(3, skinnedCBAddress);
		}
		else
		{
			cmdList->SetGraphicsRootConstantBufferView(3, 0);
		}

		cmdList->DrawIndexedInstanced(ri->IndexCount, (UINT)ri->Instances.size(), ri->StartIndexLocation, ri->BaseVertexLocation, 0);

	}
}


void Direct3D12Renderer::ClearResource()
{
	//���� �ִ� �ڿ��� ����
	//���������� ����
	for (auto& e : mAllRitems) {
		e.reset();
	}
	mAllRitems.clear();

	for (auto& e : mRitemLayer) {
		e.clear();
	}

	//������Ʈ�� ����
	for (auto& e : mStaticGeometries) {
		e.second.reset();
	}
	mStaticGeometries.clear();

	for (auto& e : mSkeletalGeometries) {
		e.second.reset();
	}
	mSkeletalGeometries.clear();

	//�ؽ��� ����
	for (auto& e : mTextures) {
		e.reset();
	}
	mTextures.clear();

	//���׸��� ����
	for (auto& e : mMaterials) {
		e.reset();
	}
	mMaterials.clear();

	//������ �ڿ� ����
	for (auto& e : mFrameResources) {
		e.reset();
	}
	mFrameResources.clear();
	mCurrFrameResource = nullptr;
	mCurrFrameResourceIndex = 0;

	mSrvDescriptorHeap.Release();
}

/*
void Direct3D12Renderer::ConvertResourceToD3D12(ResourceManager& resources)
{
	//���ҽ� �Ŵ����� ����� �����͸� ���� ������ ������ ����

	//����ƽ ����
	{
		auto& geos = resources.GetStaticGeometries();

		for (auto& e : geos) {
			BuildD3DGeometry(e.get());
		}
	}

	//���̷�Ż ����
	{
		auto& geos = resources.GetSkeletalGeometries();

		for (auto& e : geos) {
			BuildD3DGeometry(e.get());
		}
	}

	//�ؽ��������� D3D12�ؽ��� ����
	auto& texs = resources.GetTextures();
	for (auto& e : texs) {
		BuildD3DTexture(e.get());
	}

	//���׸���
	auto& mats = resources.GetMaterials();
	int matIdx = 0;
	for (auto& e : mats) {
		BuildD3DMaterial(e.get());
	}

}
*/
int Direct3D12Renderer::FindTextureIndex(std::string texName)
{
	int idx = -1;
	for (int i = 0; i < (int)mTextures.size(); ++i) {
		if (mTextures[i]->TexData->Name.compare(texName) == 0) {
			idx = i;
		}
	}
	return idx;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Direct3D12Renderer::GetCpuSrv(int index) const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Direct3D12Renderer::GetGpuSrv(int index) const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Direct3D12Renderer::GetDsv(int index) const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, mDsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Direct3D12Renderer::GetRtv(int index) const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Direct3D12Renderer::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}