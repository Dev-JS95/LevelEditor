#include "FrameResource.h"

const int gNumFrameResources = 3;

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT materialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&CmdListAlloc)));

	if (passCount > 0)
		PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);

	if (maxInstanceCount > 0)
		InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, maxInstanceCount, false);

	if (materialCount > 0)
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);


	SsaoCB = std::make_unique<UploadBuffer<SsaoConstants>>(device, 1, true);

}

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT materialCount, UINT maxSkinnedCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&CmdListAlloc)));

	if (passCount > 0)
		PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);

	if (maxInstanceCount > 0)
		InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, maxInstanceCount, false);

	if (materialCount > 0)
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);

	if (maxSkinnedCount > 0)
		SkinnedCB = std::make_unique<UploadBuffer<SkinnedConstants>>(device, maxSkinnedCount, true);

	SsaoCB = std::make_unique<UploadBuffer<SsaoConstants>>(device, 1, true);
}

FrameResource::~FrameResource()
{
	if (PassCB != nullptr)
		PassCB.reset();

	if (InstanceBuffer != nullptr)
		InstanceBuffer.reset();

	if (MaterialBuffer != nullptr)
		MaterialBuffer.reset();

	if (SkinnedCB != nullptr)
		SkinnedCB.reset();

	if (SkinnedCB != nullptr)
		SsaoCB.reset();
}

void FrameResource::SetPassCBNum(ID3D12Device* device, int newNum)
{
	if (PassCB != nullptr)
		PassCB.reset();

	if (newNum > 0)
		PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, newNum, true);
}

void FrameResource::SetInstanceBufferNum(ID3D12Device* device, int newNum)
{
	if (InstanceBuffer != nullptr)
		InstanceBuffer.reset();

	if (newNum > 0)
		InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, newNum, false);
}

void FrameResource::SetMaterialBufferNum(ID3D12Device* device, int newNum)
{
	if (MaterialBuffer != nullptr)
		MaterialBuffer.reset();

	if (newNum > 0)
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, newNum, false);
}

void FrameResource::SetSkinnedCBBufferNum(ID3D12Device* device, int newNum)
{
	if (SkinnedCB != nullptr)
		SkinnedCB.reset();

	if (newNum > 0)
		SkinnedCB = std::make_unique<UploadBuffer<SkinnedConstants>>(device, newNum, true);

}

void FrameResource::SetSsaoCBNum(ID3D12Device* device, int newNum)
{
}
