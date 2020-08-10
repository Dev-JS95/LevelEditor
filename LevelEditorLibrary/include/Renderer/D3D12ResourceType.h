#pragma once
#include "../ResourceManager/ResourceType.h"

#include <d3d12.h>
#include <atlbase.h>



struct D3D12Geometry
{
	ATL::CComPtr<ID3DBlob> VertexBufferCPU = nullptr;	//���� ���� �ý��� �޸�
	ATL::CComPtr<ID3DBlob> IndexBufferCPU = nullptr;	//�ε��� ���� �ý��� �޸�

	ATL::CComPtr<ID3D12Resource> VertexBufferGPU = nullptr;	//���� ���� ���� �޸�
	ATL::CComPtr<ID3D12Resource> IndexBufferGPU = nullptr;	//�ε��� ���� ���� �޸�

	ATL::CComPtr<ID3D12Resource> VertexBufferUploader = nullptr;	//���� ���� ���δ� �޸�
	ATL::CComPtr<ID3D12Resource> IndexBufferUploader = nullptr;	//�ε��� ���� ���δ� �޸�

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;	//���� ���� ����Ʈ ������
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;	//�ε��� ����
	UINT IndexBufferByteSize = 0;	//�ε��� ���� ����Ʈ ������

	Geometry* GeoData = nullptr;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	// GPU �޸𸮿� ���ε尡 ������ ���δ� ���۴� ����
	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

//D3D12 ����ƽ ���� ���� ����ü
struct D3D12StaticGeometry : public D3D12Geometry
{
};

//D3D12 ���̷�Ż ���� ���� ����ü
struct D3D12SkeletalGeometry : public D3D12Geometry
{
};

/*D3D12 �ؽ���*/
struct D3D12Texture
{
	Texture* TexData = nullptr;

	//�Ҵ���� �ڿ� 
	ATL::CComPtr<ID3D12Resource> Resource = nullptr;
	//�ڿ��� ���ε� ��
	ATL::CComPtr<ID3D12Resource> UploadHeap = nullptr;
};

extern const int gNumFrameResources;

/*D3D12 ����*/
struct D3D12Material
{
	Material* MatData = nullptr;

	// ������ۿ����� ���׸��� �ε���
	int MatCBIndex = -1;

	// SRV heap������ diffuse texture �ε���
	int DiffuseSrvHeapIndex = -1;
	// SRV heap������ normal texture �ε���
	int NormalSrvHeapIndex = -1;
	// SRV heap������ specular texture �ε���
	int SpecularSrvHeapIndex = -1;

	//�� ������ ���� ������ ������Ʈ �� �� �ֵ��� ����
	int NumFramesDirty = gNumFrameResources;
};

//������ ����
enum class RenderLayer : uint8_t
{
	None = 0,
	Opaque = 1,
	SkinnedOpaque,
	BoneOpaque,
	SIZE
};

/*D3D12 ��Ű�� �� ���� ����ü*/
struct D3D12SkinnedModelInstance
{
	SkinnedData* SkinnedInfo = nullptr;

	std::vector<DirectX::XMFLOAT4X4> FinalTransforms;	//������ȯ���
	std::string ClipName;	//Ŭ���̸�

	float TimePos = 0.0f;	//�ð�

	//dt������ ������ȯ��� ������Ʈ
	void UpdateSkinnedAnimation(float dt) {
		TimePos += dt;

		if (TimePos > SkinnedInfo->GetClipEndTime(ClipName))
			TimePos = 0.0f;

		SkinnedInfo->GetFinalTransforms(ClipName, TimePos, FinalTransforms);
	}
};

/*������ �׸�*/
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	//GeoMetry���� ã�� �̸�
	std::string Name;

	//�⺻���� Ÿ��
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	
	//������Ʈ�� ����
	D3D12Geometry* Geo = nullptr;

	// DrawIndexedInstanced �Ű������� ���� ����
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;

	//Instancing
	std::vector<InstanceData> Instances;
	int32_t InstanceSrvIndex = -1;
	int32_t InstanceNum = 1;

	//Bounding Box
	DirectX::BoundingBox Bounds;

	//��Ű�� �ִϸ��̼ǿ� �ʿ��� ���� ����ü
	D3D12SkinnedModelInstance* SkinnedModelInst;

	//Bone��ȯ ����� �ε���
	UINT SkinnedCBIndex;
};








