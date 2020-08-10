#pragma once
#include "FrameResourceType.h"
#include "UploadBuffer.h"
#include <memory>

extern const int gNumFrameResources;

/*������ �ڿ�*/
struct FrameResource
{
	//������
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT materialCount);
	FrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT materialCount, UINT maxSkinnedCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	//�޼���
public:
	//������ ���� ���� PassCB���� ���� (�޸� ���� �� �Ҵ����� ���� ȣ���� ���ɿ� ���� ����)
	void SetPassCBNum(ID3D12Device* device, int newNum);

	//������ ���� ���� InstanceBuffer���� ���� (�޸� ���� �� �Ҵ����� ���� ȣ���� ���ɿ� ���� ����)
	void SetInstanceBufferNum(ID3D12Device* device, int newNum);

	//������ ���� ���� MaterialBuffer���� ���� (�޸� ���� �� �Ҵ����� ���� ȣ���� ���ɿ� ���� ����)
	void SetMaterialBufferNum(ID3D12Device* device, int newNum);

	//������ ���� ���� SkinnedCBBuffer���� ���� (�޸� ���� �� �Ҵ����� ���� ȣ���� ���ɿ� ���� ����)
	void SetSkinnedCBBufferNum(ID3D12Device* device, int newNum);

	//������ ���� ���� SSAOCB���� ���� (�޸� ���� �� �Ҵ����� ���� ȣ���� ���ɿ� ���� ����)
	void SetSsaoCBNum(ID3D12Device* device, int newNum);

	//�ʵ�
public:
	ATL::CComPtr<ID3D12CommandAllocator> CmdListAlloc;

	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;	//�н���� ����
	std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;	//�ν��Ͻ� ������ ����
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;	//���׸��� ������ ����
	std::unique_ptr<UploadBuffer<SkinnedConstants>> SkinnedCB = nullptr;	//��Ų�� ��� ����
	std::unique_ptr<UploadBuffer<SsaoConstants>> SsaoCB = nullptr;	//SSAO ��� ����

	UINT64 Fence = 0;	//�潺 ��
};