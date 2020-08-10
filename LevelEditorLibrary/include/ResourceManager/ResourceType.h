#pragma once

#include "AnimationHelper.h"
#include <vector>
#include <array>
#include <unordered_map>


//정점 구조체
struct Vertex
{
	//생성자
public:
	Vertex() = default;
	Vertex(
		const DirectX::XMFLOAT3& p,
		const DirectX::XMFLOAT3& n,
		const DirectX::XMFLOAT3& b,
		const DirectX::XMFLOAT3& t,
		const DirectX::XMFLOAT2& uv) :
		Position(p),
		Normal(n),
		BiNormal(b),
		TangentU(t),
		TexC(uv) {}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		Position(px, py, pz),
		Normal(nx, ny, nz),
		TangentU(tx, ty, tz),
		TexC(u, v) {
		BiNormal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float bx, float by, float bz,
		float tx, float ty, float tz,
		float u, float v) :
		Position(px, py, pz),
		Normal(nx, ny, nz),
		BiNormal(bx, by, bz),
		TangentU(tx, ty, tz),
		TexC(u, v) {}

	bool operator==(const Vertex& other) const {
		bool posB = DirectX::XMVector3Equal(XMLoadFloat3(&Position), XMLoadFloat3(&other.Position));
		bool norB = DirectX::XMVector3Equal(XMLoadFloat3(&Normal), XMLoadFloat3(&other.Normal));
		bool binorB = DirectX::XMVector3Equal(XMLoadFloat3(&BiNormal), XMLoadFloat3(&other.BiNormal));
		bool tanB = DirectX::XMVector3Equal(XMLoadFloat3(&TangentU), XMLoadFloat3(&other.TangentU));
		bool texB = DirectX::XMVector2Equal(XMLoadFloat2(&TexC), XMLoadFloat2(&other.TexC));

		return posB && norB && binorB && tanB && texB;
	}

	//필드
public:
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 BiNormal;
	DirectX::XMFLOAT3 TangentU;
	DirectX::XMFLOAT2 TexC;

	friend struct std::hash<Vertex>;
};

//해쉬맵에 key로 정점 사용시 필요한 오버로딩
namespace std {
	template <>
	struct hash<Vertex> {
		size_t operator()(const Vertex& t) const {
			hash<float> hash_func;

			return hash_func(t.Position.x) + hash_func(t.Position.y) + hash_func(t.Position.z);
		}
	};
}


//스키닝용 정점
struct SkinnedVertex : Vertex
{
	SkinnedVertex() = default;

	SkinnedVertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		Vertex(px, py, pz,
			nx, ny, nz,
			tx, ty, tz,
			u, v) {
		BiNormal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		BoneWeights = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		memset(BoneIndices, 0x00, sizeof(BoneIndices));
	}

	DirectX::XMFLOAT3 BoneWeights;	//x, y, z, 1-(x+y+z)로 구성
	BYTE BoneIndices[4];	//연관된 뼈의 인덱스(4개까지 가능)
};


//메쉬 데이터(정점 + 인덱스)
struct MeshData abstract
{
public:
	std::vector<uint16_t>& GetIndices16()
	{
		if (mIndices16.empty())
		{
			mIndices16.resize(Indices32.size());
			for (size_t i = 0; i < Indices32.size(); ++i)
				mIndices16[i] = static_cast<uint16_t>(Indices32[i]);
		}

		return mIndices16;
	}

	//�ʵ�
public:
	std::vector<uint32_t> Indices32;	//32비트 인덱스

private:
	std::vector<uint16_t> mIndices16;	//16비트 인덱스
};

//정적 메쉬데이터
struct StaticMeshData : public MeshData
{
public:
	std::vector<Vertex> Vertices;
};

//스키닝 메쉬데이터
struct SkinnedMeshData : public MeshData
{
public:
	std::vector<SkinnedVertex> Vertices;
};


struct SubmeshGeometry
{
	std::string Name;		//Submesh Name(Key)
	std::string MatName;		//Material Name	!!!!!!!!!!!!!!!!!!!!!!!!!!!!인스턴싱을 이용해서 하면 될것같음 수정핋요!!!!!

	uint32_t IndexCount = 0;	//부분 메쉬의 인덱스 갯수
	uint32_t StartIndexLocation = 0;	//부분 메쉬의 시작 인덱스 위치
	uint32_t BaseVertexLocation = 0;	//부분 메쉬의 기준 정점 위치

	//Bounding Box
	DirectX::BoundingBox Bounds;
};



struct Resource abstract {
	std::string Name;		//키값
	bool isDirty = false;
};



//
//Geometry 에셋관련
//

struct Geometry : Resource
{
	std::vector<SubmeshGeometry> SubGeoInfos;	//부분메쉬들의 배열
	bool IsStatic = true;
};


struct StaticGeometry : public Geometry
{
	StaticMeshData Mesh;	//스태틱 메시
};


struct SkeletalGeometry : public Geometry
{
	SkinnedMeshData Mesh;	//스킨드 메시
	Skeleton* Skeleton;	//연관된 스켈레톤
};


//
//Texture 에셋관련
//

static std::vector<std::string> CanUsedTextureExtensions =
{
	".dds",
	".bmp",
	".jpeg",
	".png",
	".tiff",
	".gif",
};

struct Texture : Resource
{

};


//
//Material 에셋관련
//

enum class TextureType : uint8_t
{
	DIFFUSE,
	NORMAL,
	SPECULAR,
	EMISSIVE,
	NUM
};

struct Material : Resource
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };	//Diffuse
	DirectX::XMFLOAT3 FresnelR0 = { 0.7f, 0.7f, 0.7f };	//Fresnel
	float Roughness = 0.8f;	//거칠기
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();	//MatTransform

	//텍스쳐
	std::string TexName[(int)TextureType::NUM];
};




//
//맵 에셋 관련
//

struct GeomtryInstanceData {
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	std::string MaterialName;
};

struct ObjectInMap {
	std::string GeometryName;
	std::vector<GeomtryInstanceData> Instances;
};

struct Map : Resource {
	//맵에는 Geomtry와 그의 인스턴싱 데이터들이 위치해 있음
	//그를 위해 위치, 메테리얼 데이터를 에셋 파일에 저장해둠
	std::vector<ObjectInMap> Objects;
};

