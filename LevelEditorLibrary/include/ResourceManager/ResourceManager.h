#pragma once
#include "ResourceType.h"

#define __RAPID_JSON__

#ifdef __RAPID_JSON__
#include "../Core/rapidjson/document.h"
#include "../Core/rapidjson/prettywriter.h" 
#include "../Core/rapidjson/error/en.h"
#include "../Core/rapidjson/filereadstream.h"
#include "../Core/rapidjson/filewritestream.h"
#include "../Core/rapidjson/writer.h"
#else
#include "../Core/Document.h"
#endif

#include <functional>
#include <filesystem> // C++17 standard header file name

class ResourceManager {

	//생성자
public:
	static ResourceManager* GetInstance();

	~ResourceManager();

private:
	ResourceManager();

	//메서드
public:
	void CreateMaterialAsset(std::filesystem::path fileName);	//���׸��� ���� ����Ʈ �������� ����

	void LoadAsset(std::filesystem::path assetPath);	//�Էµ� ����� ������ �ε��Ͽ� ���� Ÿ�Կ� �°� �迭�� ����

	//맵 에셋에 존재하는 Geomtry, Material Texture 에셋을 연쇄적으로 불러온다
	void LoadMap(std::filesystem::path mapAsset = "Assets/DefaultMap.asset");
	//현재 로드된 맵 에셋에 존재하는 geomtry, Material Texture 에셋을 연쇄적으로 저장
	void SaveMap(std::filesystem::path mapAsset);	



	void SaveMaterialAsset(std::filesystem::path assetPath, int geoIdx, int submeshIdx);	//�Էµ� ��η� ���� geo + subMesh�� ���׸��� ����

	void ChangeMaterialGeo(std::string MatName, int geoIdx, int submeshIdx);	//������ Geomtery�� SubMesh ���׸��� ü����
	void ChangeTextureMap(std::string textureName, TextureType type, int geoIdx, int submeshIdx);	//������ Geometry�� submesh�� ���׸����� Diffuse����

	void ExportGeometryAsset(std::string& fileName, int geoIdx);	//Geometry Asset 저장, geoIdx로 설정
	void ExportSkeletonAsset(std::string& fileName, int skeletonIdx);	//���̷��� ���� �ͽ���Ʈ
	void ExportAnimationAsset(std::string& fileName, int skeletonIdx);	//�ִϸ��̼� ���� �ͽ���Ʈ

	void ImportGeometryAsset(const std::filesystem::path& filePath);	//Geometry Asset 추가 / Geo가 사용중인 Material 추가로 로드
	void ImportMaterialAsset(const std::filesystem::path& filePath);	//Material Asset 추가 / Material이 사용중인 Texture추가로 로드
	void ImportTextureAsset(const std::filesystem::path& filePath);		//Texture Asset 추가

	void ImportSkeletonAsset(std::string& fileName);	//���̷��� ���� ����Ʈ
	void ImportAnimationAsset(std::string& fileName);	//�ִϸ��̼� ���� ����Ʈ

	void ExportGeometryAsset(const std::string& geoName);	//Geometry Asset 저장 / Geo가 사용중인 Material 추가로 저장
	void ExportMaterialAsset(const std::string& matName);	//Material Asset 저장



	std::vector<std::unique_ptr<StaticGeometry>>& GetStaticGeometries() { return mStaticGeometries; }
	std::vector<std::unique_ptr<SkeletalGeometry>>& GetSkeletalGeometries() { return mSkeletalGeometries; }
	std::vector<std::unique_ptr<Texture>>& GetTextures() { return mTextures; }
	std::vector<std::unique_ptr<Material>>& GetMaterials() { return mMaterials; }
	std::vector<std::unique_ptr<Skeleton>>& GetSkeletons() { return mSkeletons; }
	std::unique_ptr<std::unordered_map<std::string, AnimationClip>>& GetAnimations() { return mAnimations; }

	int FindMaterialIndexForName(std::string matName);	//���׸��� �̸����� �ε��� �˻�. �� ã���� -1 ����

	void SetMaterials(std::vector<std::unique_ptr<Material>>& mats) { mMaterials = move(mats); }
	void SetTextures(std::vector<std::unique_ptr<Texture>>& texs) { mTextures = move(texs); }
	void SetAnimations(std::unique_ptr<std::unordered_map<std::string, AnimationClip>>& anims) { mAnimations = move(anims); }
	void SetUseDefaultShape(bool set) { bCanUseDefaultShapes = set; }

private:
	static void LoadMapAsset(rapidjson::Document& doc, ResourceManager&);
	static void LoadGeometryAsset(rapidjson::Document& doc, ResourceManager&);
	static void LoadTextureAsset(std::filesystem::path& path, ResourceManager&);
	static void LoadMaterialAsset(rapidjson::Document& doc, ResourceManager&);
	static void LoadSkeletonAsset(rapidjson::Document& doc, ResourceManager&);
	static void LoadAnimationAsset(rapidjson::Document& doc, ResourceManager&);

	//에셋파일 하나씩 불러오기
	/*
	static void ImportMapAsset(rapidjson::Document& doc, ResourceManager&);
	static void ImportGeometryAsset(rapidjson::Document& doc, ResourceManager&);
	static void ImportTextureAsset(std::filesystem::path& path, ResourceManager&);
	static void ImportMaterialAsset(rapidjson::Document& doc, ResourceManager&);
	static void ImportSkeletonAsset(rapidjson::Document& doc, ResourceManager&);
	static void ImportAnimationAsset(rapidjson::Document& doc, ResourceManager&);
	*/
	void CreateMaterialAsset(std::string& fileName, int geoIdx);	//���׸��� ���� ����
	void CreateGeometryAsset(std::string& fileName, int geoIdx);	//Geo���� ����

	void BuildMaterial(std::string& fileName);	//���׸��� ���

	void BuildDefaultGeometry();	//�⺻���� ���

	void ClearResource();	//�ڿ��� ����


	//�ʵ�
public:
	bool bCanUseDefaultShapes = false;

private:
	static ResourceManager* mInstance;
	
	std::unique_ptr<Map> mMap;	//맵 데이터
	std::vector<std::unique_ptr<StaticGeometry>> mStaticGeometries;	//정적기하
	std::vector<std::unique_ptr<SkeletalGeometry>> mSkeletalGeometries;	//스케레탈 기하
	std::vector<std::unique_ptr<Material>> mMaterials;	//메테리얼
	std::vector<std::unique_ptr<Texture>> mTextures;	//텍스쳐

	std::vector<std::unique_ptr<Skeleton>> mSkeletons;	//스켈레톤

	using AnimationNameClip = std::unordered_map<std::string, AnimationClip>;
	std::unique_ptr<AnimationNameClip> mAnimations;	//애니메이션 맵


	std::vector<std::string> mAssetType = {
	"Map"
	"StaticGeometry",
	"SkeletalGeometry",
	"Material",
	"Skeleton",
	"Animation" };

	std::vector<std::function<void(rapidjson::Document&, ResourceManager&)>> mAssetParsingFunc = {
		LoadMapAsset,
		LoadGeometryAsset,
		LoadGeometryAsset,
		LoadMaterialAsset,
		LoadSkeletonAsset,
		LoadAnimationAsset
	};

};
