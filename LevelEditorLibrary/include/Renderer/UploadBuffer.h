#pragma once
#include "D3D12RendererUtils.h"

template<typename T>
class UploadBuffer
{
	//������
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);
		mElementCount = elementCount;
		// ��������� ��Ҵ� 256����Ʈ�� ��������Ѵ�
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			mElementByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(T));

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

		// ���ҽ� ���� ��������� Unmap�� �ϸ� �ȵ�. 
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	//�޼ҵ�
public:
	//���ε� ���� ����
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer;
	}

	//���ۿ� ������ ����
	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex*mElementByteSize], &data, sizeof(T));
	}

	//���� �ʱ�ȭ
	void Clear() {
		memset(mMappedData, 0x00, mElementByteSize * mElementCount);
	}

	//�ʵ�
private:
	ATL::CComPtr<ID3D12Resource> mUploadBuffer;	//���ε� ����
	BYTE* mMappedData = nullptr;	//���ε� ����

	UINT mElementByteSize = 0;	//��� ����Ʈ ������
	UINT mElementCount = 0;	//��� ����
	bool mIsConstantBuffer = false;	//������� �οﰪ
};