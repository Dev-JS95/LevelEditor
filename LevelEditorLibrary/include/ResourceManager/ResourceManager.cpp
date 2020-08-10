#include "ResourceManager.h"
#include "GeometryGenerator.h"

#include <io.h>		//access
#include <iostream>
#include <regex>

using namespace rapidjson;
using namespace std;
using namespace DirectX;
using namespace std::filesystem;

ResourceManager* ResourceManager::mInstance = nullptr;

ResourceManager* ResourceManager::GetInstance()
{
	if (mInstance == nullptr)
		mInstance = new ResourceManager();

	return mInstance;
}

ResourceManager::ResourceManager()
{
	//Defualt Mat 생성
	auto defaultMat = make_unique<Material>();
	defaultMat.get()->Name = "Assets/DefaultMat";
	mMaterials.push_back(move(defaultMat));

	//기본도형 생성
	if (bCanUseDefaultShapes)
		BuildDefaultGeometry();
}


ResourceManager::~ResourceManager()
{
	ClearResource();

	delete mInstance;
}

void ResourceManager::CreateMaterialAsset(std::filesystem::path fileName)
{
	//
	//���׸��� ���� ����
	//
	Material mat;
	mat.Name = fileName.stem().string();

	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//Ÿ�� ����
	//
	doc.AddMember("Type", "Material", alloc);

	//
	//�̸� ����
	//
	Value nameValue(mat.Name.c_str(), (rapidjson::SizeType)mat.Name.size(), alloc);
	doc.AddMember("Name", nameValue, alloc);

	//
	//�ؽ��� ��θ� ����
	//
	std::filesystem::path textureFolder = fileName.parent_path().string() + "\\textures";	//���ο� �ؽ��İ� ����� ���

	if (!exists(textureFolder)) {
		cout << "Not Have Texture Directory, Create....." << endl;
		create_directory(textureFolder);
	}

	Value texArrayValue(kArrayType);
	texArrayValue.SetObject();
	for (int texIdx = 0; texIdx < (int)TextureType::NUM; ++texIdx) {	//�ؽ��� �迭�� ����ŭ
		if (mat.TexName[texIdx].empty())
			continue;

		for (int j = 0; j < (int)mTextures.size(); ++j) {	//�ؽ��� �迭�� �ؽ��İ� �����ϴ��� �˻�

			if (mTextures[j]->Name.compare(mat.TexName[texIdx]) == 0) {	//���� �̸��� �ؽ��İ� �����ϸ� ����
				std::string texType;
				switch (texIdx) {
				case (int)TextureType::DIFFUSE:
					texType = "diffuse";
					break;
				case (int)TextureType::NORMAL:
					texType = "normal";
					break;
				case (int)TextureType::SPECULAR:
					texType = "specular";
					break;
				case (int)TextureType::EMISSIVE:
					texType = "emissive";
					break;
				default:
					texType = "";
					break;
				}
				std::string texFileNamePath = mTextures[j]->Name;	//�ؽ��� ���
				int nameIdx = (int)texFileNamePath.find_last_of("\\");
				std::string texFileName = texFileNamePath.substr(nameIdx, texFileNamePath.size());

				regex r(R"((?:[^|*?\/:<>"])+)");
				auto beginIter = sregex_iterator(texFileName.begin(), texFileName.end(), r);
				auto endIter = sregex_iterator();
				int numIter = (int)std::distance(beginIter, endIter);

				for (std::sregex_iterator i = beginIter; i != endIter; ++i) {

					if (numIter == 1) {
						std::smatch match = *i;
						texFileName = match.str();
					}

					numIter--;
				}

				std::string newTexFileName = textureFolder.string() + "\\" + texFileName;	//������ �ؽ��� ���

				//
				//���� ����
				//
				if (texFileNamePath.compare(newTexFileName) != 0) {	//���� �ؽ��� ���ϰ� ���ο� �ؽ��� ������ �ٸ� ��츸 ���� ����
					FILE* texFile, * newTexFile;
					fopen_s(&texFile, texFileNamePath.c_str(), "rb");
					fopen_s(&newTexFile, newTexFileName.c_str(), "wb");

					//
					//���۸� �̿��Ͽ� ����
					//
					char buf[1024];
					while (!feof(texFile))
					{
						fread(buf, sizeof(char), sizeof(buf), texFile);
						fwrite(buf, sizeof(char), sizeof(buf), newTexFile);
					}

					fflush(newTexFile);	//�÷���

					fclose(texFile);
					fclose(newTexFile);
				}

				//�ؽ��� Ÿ�Ժ��� ����
				Value arrKey(texType.c_str(), (rapidjson::SizeType)texType.size(), alloc);
				Value arrValue(newTexFileName.c_str(), (rapidjson::SizeType)newTexFileName.size(), alloc);
				texArrayValue.AddMember(arrKey, arrValue, alloc);

				break;
			}

		}

	}
	doc.AddMember("Textures", texArrayValue, alloc);

	//
	//DiffuseAlbedo
	//
	Value diffuseAlbedoValue(kArrayType);
	diffuseAlbedoValue.PushBack(mat.DiffuseAlbedo.x, alloc);
	diffuseAlbedoValue.PushBack(mat.DiffuseAlbedo.y, alloc);
	diffuseAlbedoValue.PushBack(mat.DiffuseAlbedo.z, alloc);
	diffuseAlbedoValue.PushBack(mat.DiffuseAlbedo.w, alloc);
	doc.AddMember("DiffuseAlbedo", diffuseAlbedoValue, alloc);

	//
	//FresnelR0
	//
	Value fresnelValue(kArrayType);
	fresnelValue.PushBack(mat.FresnelR0.x, alloc);
	fresnelValue.PushBack(mat.FresnelR0.y, alloc);
	fresnelValue.PushBack(mat.FresnelR0.z, alloc);
	doc.AddMember("FresnelR0", fresnelValue, alloc);

	//
	//Roughness
	//
	Value roughnessValue(mat.Roughness);
	doc.AddMember("Roughness", roughnessValue, alloc);

	//
	//MatTransform
	//
	Value matTrValue(kArrayType);
	float* it = (float*)&mat.MatTransform;
	for (uint32_t i = 0; i < 16; ++i) {
		Value matValue(*(it + i));
		matTrValue.PushBack(matValue, alloc);
	}
	doc.AddMember("MatTransform", matTrValue, alloc);


	//
	//���� ���� ����
	//
	FILE* matAsset;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);

	fopen_s(&matAsset, fileName.string().c_str(), "wt");

	if (matAsset != nullptr) {
		fputs(sb.GetString(), matAsset);
		fflush(matAsset);
		fclose(matAsset);
	}
}

void ResourceManager::LoadAsset(std::filesystem::path assetPath)
{
	bool isTextureFile = false;
	//�ؽ��� �������� üũ
	for (size_t i = 0; i < CanUsedTextureExtensions.size(); ++i) {
		if (CanUsedTextureExtensions[i].compare(assetPath.extension().string()) == 0) {
			isTextureFile = true;
			break;
		}
	}

	//�ؽ��� ������ ���
	if (isTextureFile) {
		LoadTextureAsset(assetPath, *this);
	}

	//�ؽ��� ������ �ƴ� ������ ���
	else {
	//
	//������ ��ο� ���� ���� �ε�
	//
		FILE* assetFile;
		fopen_s(&assetFile, assetPath.string().c_str(), "rb");

		if (assetFile == nullptr) {
			cout << "AssetFile Open Error !" << endl;
			return;
		}

		//
		//���� ũ�� �˾Ƴ���
		//
		fseek(assetFile, 0, SEEK_END);
		int nFileSize = ftell(assetFile);
		fseek(assetFile, 0, SEEK_SET);

		char* buf = new char[nFileSize];	//���� ũ�⿡ ���� ���� ����

		FileReadStream is(assetFile, buf, nFileSize);	//��Ʈ�� ����

		Document doc;
		ParseResult ok = doc.ParseStream(is);	//��Ʈ������ JSON�Ľ�

		fclose(assetFile);	//���� �ݱ�

		if (!ok) {	//�Ľ��� �����ߴٸ�
			std::cout << "JSON parse error: " << GetParseError_En(ok.Code()) << ok.Offset() << std::endl;
		}


		//
		//JSON ���� �������� �Ľ�
		//
		assert(doc.IsObject());

		//
		//Type ��������
		//
		assert(doc.HasMember("Type"));
		assert(doc["Type"].IsString());

		std::string type = doc["Type"].GetString();

		//
		//Type�� ���� ���� �� �Ľ� �Լ� ȣ��
		//
		for (size_t i = 0; i < mAssetType.size(); ++i) {
			if (type.compare(mAssetType[i]) == 0) {
				mAssetParsingFunc[i](doc, *this);
				break;
			}
		}
	}

}

void ResourceManager::LoadMap(std::filesystem::path mapAsset)
{
	//에셋파일 읽기
	FILE* assetFile;
	fopen_s(&assetFile, mapAsset.string().c_str(), "rb");

	if (assetFile == nullptr) {
		throw exception("AssetFile Open Error !");
		return;
	}

	//
	//파일 읽기
	//
	fseek(assetFile, 0, SEEK_END);
	int nFileSize = ftell(assetFile);
	fseek(assetFile, 0, SEEK_SET);

	char* buf = new char[nFileSize];	//���� ũ�⿡ ���� ���� ����

	FileReadStream is(assetFile, buf, nFileSize);	//��Ʈ�� ����

	Document doc;
	ParseResult ok = doc.ParseStream(is);	//��Ʈ������ JSON 파싱

	fclose(assetFile);	//���� �ݱ�

	if (!ok) {	//�Ľ��� �����ߴٸ�
		char str[256] = { 0, };
		sprintf_s(str, "JSON parse error: %s %d \n", GetParseError_En(ok.Code()), (int)ok.Offset());
		throw exception(str);
		return;
	}

	//
	//JSON 오브젝트인지 체크
	//
	assert(doc.IsObject());

	//
	//Type 확인
	//
	assert(doc.HasMember("Type"));
	assert(doc["Type"].IsString());
	std::string type = doc["Type"].GetString();

	//
	//Type이 Map이 아닌 경우 에러
	//
	if (type.compare("Map") != 0) {
		throw exception("Type Is Not Map!!");
		return;
	}

	//값을 저장할 임시 맵
	std::unique_ptr<Map> tempMap = std::make_unique<Map>();

	//
	//Name 확인
	//
	assert(doc.HasMember("Name"));
	assert(doc["Name"].IsString());
	tempMap->Name = doc["Name"].GetString();
	
	//
	//Objects 확인
	//
	assert(doc.HasMember("Objects"));
	assert(doc["Objects"].IsArray());
	auto objects = doc["Objects"].GetArray();
	
	//오브젝트 데이터 가져오기
	for (const auto& e : objects) {
		ObjectInMap obj;

		//Geometry에셋 가져오기
		assert(e.HasMember("GeometryAsset"));
		assert(e["GeometryAsset"].IsString());
		std::filesystem::path geoAssetName = e["GeometryAsset"].GetString();
		
		ImportGeometryAsset(geoAssetName.relative_path());
		obj.GeometryName = geoAssetName.string();

		//인스턴스 데이터 가져오기
		assert(e.HasMember("Instances"));
		assert(e["Instances"].IsArray());
		auto instances = e["Instances"].GetArray();
		
		for (const auto& inst : instances) {
			GeomtryInstanceData geoInst;

			//메테리얼 이름
			assert(inst.HasMember("MaterialAsset"));
			assert(inst["MaterialAsset"].IsString());
			std::filesystem::path matName = inst["MaterialAsset"].GetString();
			geoInst.MaterialName = matName.string();

			//World Tr
			assert(doc.HasMember("World"));
			assert(doc["World"].IsArray());
			auto worldTr = doc["World"].GetArray();
			float* worldIt = (float*)&geoInst.World;
			for (int i = 0; i < (int)worldTr.Size(); ++i) {
				*(float*)(worldIt + i) = worldTr[i].GetFloat();
			}

			//Tex Tr
			assert(doc.HasMember("TexTransform"));
			assert(doc["TexTransform"].IsArray());
			auto texTr = doc["TexTransform"].GetArray();
			float* texIt = (float*)&geoInst.TexTransform;
			for (int i = 0; i < (int)texTr.Size(); ++i) {
				*(float*)(texIt + i) = texTr[i].GetFloat();
			}

			obj.Instances.push_back(geoInst);
		}

		tempMap->Objects.push_back(obj);
	}

	tempMap->isDirty = false;

	mMap = move(tempMap);

	doc.RemoveAllMembers();
}

void ResourceManager::SaveMap(std::filesystem::path mapAsset)
{
	if (mMap == nullptr) {
		throw exception("참조된 Map이 없습니다");
		return;
	}

	//깨끗한 경우 저장할 필요없음
	if (!mMap->isDirty)
		return;

	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//Type 설정
	//
	doc.AddMember("Type", "Map", alloc);

	//
	//Name 설정
	//
	Value nameValue(mMap->Name.c_str(), (rapidjson::SizeType)mMap->Name.size(), alloc);
	doc.AddMember("Name", nameValue, alloc);

	//
	//맵에 있는 오브젝트 설정
	//
	Value objectArrayValue(kArrayType);
	objectArrayValue.SetArray();
	for (const auto& e : mMap->Objects) {
		Value objectValue(kObjectType);
		objectValue.SetObject();

		//geomtry이름
		Value geoNameValue(e.GeometryName.c_str(), (rapidjson::SizeType)e.GeometryName.size(), alloc);

		ExportGeometryAsset(e.GeometryName);

		//인스턴싱 데이터
		Value instanceArrayValue(kArrayType);
		instanceArrayValue.SetArray();
		for (const auto& inst : e.Instances) {
			Value instObjectValue(kObjectType);
			instObjectValue.SetObject();

			//메테리얼 이름
			Value matNameValue(inst.MaterialName.c_str(), (rapidjson::SizeType)inst.MaterialName.size(), alloc);

			//4x4 월드 행렬
			Value matWorldValue(kArrayType);
			matWorldValue.SetArray();
			float* worldIt = (float*)&inst.World;
			for (uint32_t i = 0; i < 16; ++i) {
				Value matValue(*(worldIt + i));
				matWorldValue.PushBack(matValue, alloc);
			}

			//4x4 텍스쳐 트랜스폼 행렬
			Value matTexTrValue(kArrayType);
			matTexTrValue.SetArray();
			float* texIt = (float*)&inst.TexTransform;
			for (uint32_t i = 0; i < 16; ++i) {
				Value matValue(*(texIt + i));
				matTexTrValue.PushBack(matValue, alloc);
			}

			instObjectValue.AddMember("MaterialAsset", matNameValue, alloc);
			instObjectValue.AddMember("World", matWorldValue, alloc);
			instObjectValue.AddMember("TexTransform", matTexTrValue, alloc);

			instanceArrayValue.PushBack(instObjectValue, alloc);
		}

		objectValue.AddMember("GeometryAsset", geoNameValue, alloc);
		objectValue.AddMember("Instances", instanceArrayValue, alloc);

		objectArrayValue.PushBack(objectValue, alloc);
	}

	doc.AddMember("Objects", objectArrayValue, alloc);

	//
	//에셋 파일로 저장
	//
	FILE* assetFile;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);

	fopen_s(&assetFile, mapAsset.string().c_str(), "wt");

	if (assetFile != nullptr) {
		fputs(sb.GetString(), assetFile);
		fclose(assetFile);
	}

	doc.RemoveAllMembers();
}

void ResourceManager::SaveMaterialAsset(std::filesystem::path assetPath, int geoIdx, int submeshIdx)
{
	if (!((size_t)geoIdx < mStaticGeometries.size() + mSkeletalGeometries.size()))
		return;

	//
	//�ؽ��� ���� ����
	//
	std::filesystem::path textureFolder = assetPath.parent_path().string() + "\\textures";	//���ο� �ؽ��İ� ����� ���

	if (!exists(textureFolder)) {
		create_directory(textureFolder);
	}

	//
	//�ش� Geometry��������
	//
	Geometry* geo = nullptr;
	if ((size_t)geoIdx < mStaticGeometries.size()) {
		geo = mStaticGeometries[geoIdx].get();
		geo->IsStatic = true;
	}
	else {
		geo = mSkeletalGeometries[geoIdx - mStaticGeometries.size()].get();
		geo->IsStatic = false;
	}

	//�ش� ����޽�
	auto subMesh = geo->SubGeoInfos[submeshIdx];

	//
	//���׸��� ���� ����
	//

	//���׸��� �˻�
	Material* mat = nullptr;
	for (auto& e : mMaterials) {
		if (e->Name.compare(subMesh.MatName) == 0) {
			mat = e.get();
			break;
		}
	}

	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//Ÿ�� ����
	//
	doc.AddMember("Type", "Material", alloc);

	//
	//�̸� ����
	//
	mat->Name = assetPath.stem().string();
	Value nameValue(mat->Name.c_str(), (rapidjson::SizeType)mat->Name.size(), alloc);
	doc.AddMember("Name", nameValue, alloc);

	//
	//�ؽ��� ��θ� ����
	//
	Value texArrayValue(kArrayType);
	texArrayValue.SetObject();
	for (int texIdx = 0; texIdx < (int)TextureType::NUM; ++texIdx) {	//�ؽ��� �迭�� ����ŭ
		if (mat->TexName[texIdx].empty())
			continue;

		for (int j = 0; j < (int)mTextures.size(); ++j) {	//�ؽ��� �迭�� �ؽ��İ� �����ϴ��� �˻�

			if (mTextures[j]->Name.compare(mat->TexName[texIdx]) == 0) {	//���� �̸��� �ؽ��İ� �����ϸ� ����
				std::string texType;
				switch (texIdx) {
				case (int)TextureType::DIFFUSE:
					texType = "diffuse";
					break;
				case (int)TextureType::NORMAL:
					texType = "normal";
					break;
				case (int)TextureType::SPECULAR:
					texType = "specular";
					break;
				case (int)TextureType::EMISSIVE:
					texType = "emissive";
					break;
				default:
					texType = "";
					break;
				}
				std::string texFileNamePath = mTextures[j]->Name;	//�ؽ��� ���
				int nameIdx = (int)texFileNamePath.find_last_of("\\");
				std::string texFileName = texFileNamePath.substr(nameIdx+1, texFileNamePath.size());

				regex r(R"((?:[^|*?\/:<>"])+)");
				auto beginIter = sregex_iterator(texFileName.begin(), texFileName.end(), r);
				auto endIter = sregex_iterator();
				int numIter = (int)std::distance(beginIter, endIter);

				for (std::sregex_iterator i = beginIter; i != endIter; ++i) {

					if (numIter == 1) {
						std::smatch match = *i;
						texFileName = match.str();
					}

					numIter--;
				}

				std::string newTexFileName = textureFolder.string() + "\\" + texFileName;	//������ �ؽ��� ���

				//
				//���� ����
				//
				if (texFileNamePath.compare(absolute(newTexFileName).string()) != 0) {	//���� �ؽ��� ���ϰ� ���ο� �ؽ��� ������ �ٸ� ��츸 ���� ����
					FILE* texFile, * newTexFile;
					fopen_s(&texFile, texFileNamePath.c_str(), "rb");
					fopen_s(&newTexFile, newTexFileName.c_str(), "wb");

					//
					//���۸� �̿��Ͽ� ����
					//
					char buf[1024];
					while (!feof(texFile))
					{
						fread(buf, sizeof(char), sizeof(buf), texFile);
						fwrite(buf, sizeof(char), sizeof(buf), newTexFile);
					}

					fflush(newTexFile);	//�÷���

					fclose(texFile);
					fclose(newTexFile);
				}

				//�ؽ��� Ÿ�Ժ��� ����
				Value arrKey(texType.c_str(), (rapidjson::SizeType)texType.size(), alloc);
				Value arrValue(newTexFileName.c_str(), (rapidjson::SizeType)newTexFileName.size(), alloc);
				texArrayValue.AddMember(arrKey, arrValue, alloc);

				break;
			}

		}

	}
	doc.AddMember("Textures", texArrayValue, alloc);

	//
	//DiffuseAlbedo
	//
	Value diffuseAlbedoValue(kArrayType);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.x, alloc);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.y, alloc);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.z, alloc);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.w, alloc);
	doc.AddMember("DiffuseAlbedo", diffuseAlbedoValue, alloc);

	//
	//FresnelR0
	//
	Value fresnelValue(kArrayType);
	fresnelValue.PushBack(mat->FresnelR0.x, alloc);
	fresnelValue.PushBack(mat->FresnelR0.y, alloc);
	fresnelValue.PushBack(mat->FresnelR0.z, alloc);
	doc.AddMember("FresnelR0", fresnelValue, alloc);

	//
	//Roughness
	//
	Value roughnessValue(mat->Roughness);
	doc.AddMember("Roughness", roughnessValue, alloc);

	//
	//MatTransform
	//
	Value matTrValue(kArrayType);
	float* it = (float*)&mat->MatTransform;
	for (uint32_t i = 0; i < 16; ++i) {
		Value matValue(*(it + i));
		matTrValue.PushBack(matValue, alloc);
	}
	doc.AddMember("MatTransform", matTrValue, alloc);


	//
	//���� ���� ����
	//
	FILE* matAsset;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);

	fopen_s(&matAsset, assetPath.string().c_str(), "wt");

	if (matAsset != nullptr) {
		fputs(sb.GetString(), matAsset);
		fclose(matAsset);
	}

}


void ResourceManager::ChangeMaterialGeo(std::string MatName, int geoIdx, int submeshIdx)
{
	Geometry* geo = nullptr;

	//
	//�� ������ Geo�� �� Idx�� �����ϱ� ���ؼ� �˻�
	//
	if (!((size_t)geoIdx < mStaticGeometries.size() + mSkeletalGeometries.size()))
		return;

	if ((size_t)geoIdx < mStaticGeometries.size()) {
		geo = mStaticGeometries[geoIdx].get();
	}
	else {
		geo = mSkeletalGeometries[geoIdx - mStaticGeometries.size()].get();
	}

	assert(geo != nullptr);

	if ((size_t)submeshIdx < geo->SubGeoInfos.size()) {
		geo->SubGeoInfos[submeshIdx].MatName = MatName;
	}
	
}

void ResourceManager::ChangeTextureMap(std::string textureName, TextureType type, int geoIdx, int submeshIdx)
{
	Geometry* geo = nullptr;

	//
	//�� ������ Geo�� �� Idx�� �����ϱ� ���ؼ� �˻�
	//
	if (!((size_t)geoIdx < mStaticGeometries.size() + mSkeletalGeometries.size()))
		return;

	if ((size_t)geoIdx < mStaticGeometries.size()) {
		geo = mStaticGeometries[geoIdx].get();
	}
	else {
		geo = mSkeletalGeometries[geoIdx - mStaticGeometries.size()].get();
	}

	assert(geo != nullptr);

	if ((size_t)submeshIdx < geo->SubGeoInfos.size()) {
		int matIdx = FindMaterialIndexForName(geo->SubGeoInfos[submeshIdx].MatName);
		mMaterials[matIdx]->TexName[(int)type] = textureName;			//�Ŀ� �迭�ε����� �Ű������� �޾Ƽ� Normal�̳� Specular�� �Ϲ�ȭ
		mMaterials[matIdx]->isDirty = true;
	}
}

void ResourceManager::ExportGeometryAsset(std::string& fileName, int geoIdx)
{
	//Geo ���� ����
	CreateGeometryAsset(fileName, geoIdx);

	//���׸��� ���� ����
	CreateMaterialAsset(fileName, geoIdx);
}

void ResourceManager::ExportSkeletonAsset(std::string& fileName, int skeletonIdx)
{
	std::string path = fileName.substr(0, fileName.find_last_of('\\'));
	std::string path_name = fileName.substr(0, fileName.find_last_of('.'));

	if (!((size_t)skeletonIdx < mSkeletons.size()))
		return;

	//
	//���̷��� ���� ����
	//
	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//Ÿ�� ����
	//
	doc.AddMember("Type", "Skeleton", alloc);

	auto* skeleton = mSkeletons[skeletonIdx].get();

	Value mSkeletonValue(kArrayType);
	mSkeletonValue.SetArray();

	for (size_t i = 0; i < skeleton->mJoints.size(); ++i) {
		auto curJoint = skeleton->mJoints[i];

		Value jointValue;
		jointValue.SetObject();
		//�θ� ����Ʈ �ε���
		//
		jointValue.AddMember("ParentIndex", curJoint.mParentIndex, alloc);

		//�̸�
		//
		Value jointNameValue(curJoint.mName.c_str(), (rapidjson::SizeType)curJoint.mName.length(), alloc);
		jointValue.AddMember("Name", jointNameValue, alloc);

		//���ε�Ÿ�� Ʈ������
		//
		Value bindTransformValue(kArrayType);
		float* it = (float*)&curJoint.mGlobalBindposeInverse;
		for (uint32_t i = 0; i < 16; ++i) {
			bindTransformValue.PushBack(*(it + i), alloc);
		}
		jointValue.AddMember("BindTime-Transform", bindTransformValue, alloc);


		mSkeletonValue.PushBack(jointValue, alloc);
	}

	doc.AddMember("Joints", mSkeletonValue, alloc);


	//
	//���� ���� ����
	//
	FILE* skeletonAsset;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);    // Accept() traverses the DOM and generates Handler events.

	std::string skeltonFileName = path_name + "_Skeleton.asset";
	fopen_s(&skeletonAsset, skeltonFileName.c_str(), "wt");

	if (skeletonAsset != nullptr) {
		fputs(sb.GetString(), skeletonAsset);
		fclose(skeletonAsset);
	}
}

void ResourceManager::ExportAnimationAsset(std::string& fileName, int skeletonIdx)
{
	std::string path = fileName.substr(0, fileName.find_last_of('\\'));
	std::string path_name = fileName.substr(0, fileName.find_last_of('.'));

	if (!((size_t)skeletonIdx < mSkeletons.size()))
		return;

	//
	//�ִϸ��̼� ���� ����
	//
	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//Ÿ�� ����
	//
	doc.AddMember("Type", "Animation", alloc);

	//
	//�ش� ���̷��� ����
	//
	auto skeletonFileName = path_name + "_Skeleton.asset";
	Value nameValue(skeletonFileName.c_str(), (rapidjson::SizeType)skeletonFileName.length(), alloc);
	doc.AddMember("SkeletonAsset", nameValue, alloc);


	//
	//Ŭ�� ����
	//
	Value animClipsValue;
	animClipsValue.SetArray();

	for (auto& e : *mAnimations) {
		Value clipValue(kArrayType);
		clipValue.SetArray();

		//BoneAnimations
		for (size_t i = 0; i < e.second.BoneAnimations.size(); ++i) {
			auto curBoneAnim = e.second.BoneAnimations[i];
			Value boneValue;
			boneValue.SetArray();

			//Keyframes
			for (size_t j = 0; j < curBoneAnim.Keyframes.size(); ++j) {
				Value keyFrameValue;
				keyFrameValue.SetObject();

				auto curKeyframe = curBoneAnim.Keyframes[j];

				//Timepos
				Value timePosValue(curKeyframe.TimePos);
				keyFrameValue.AddMember("TimePos", timePosValue, alloc);

				//Translation
				Value translationValue(kArrayType);
				translationValue.PushBack(curKeyframe.Translation.x, alloc);
				translationValue.PushBack(curKeyframe.Translation.y, alloc);
				translationValue.PushBack(curKeyframe.Translation.z, alloc);
				keyFrameValue.AddMember("Translation", translationValue, alloc);

				//Quaternion
				curKeyframe.RotationQuat;
				Value quaternionValue(kArrayType);
				quaternionValue.PushBack(curKeyframe.RotationQuat.x, alloc);
				quaternionValue.PushBack(curKeyframe.RotationQuat.y, alloc);
				quaternionValue.PushBack(curKeyframe.RotationQuat.z, alloc);
				quaternionValue.PushBack(curKeyframe.RotationQuat.w, alloc);
				keyFrameValue.AddMember("Quaternion", quaternionValue, alloc);

				//Scale
				Value scaleValue(kArrayType);
				scaleValue.PushBack(curKeyframe.Scale.x, alloc);
				scaleValue.PushBack(curKeyframe.Scale.y, alloc);
				scaleValue.PushBack(curKeyframe.Scale.z, alloc);
				keyFrameValue.AddMember("Scale", scaleValue, alloc);

				boneValue.PushBack(keyFrameValue, alloc);
			}

			clipValue.PushBack(boneValue, alloc);
		}

		//AnimName
		Value clipNameValue(e.first.c_str(), (SizeType)e.first.length(), alloc);

		//�̸� - Ŭ�� �� ������Ʈ�� ����
		Value animClipElement;
		animClipElement.SetObject();
		animClipElement.AddMember("Name", clipNameValue, alloc);
		animClipElement.AddMember("ClipData", clipValue, alloc);

		animClipsValue.PushBack(animClipElement, alloc);
	}

	doc.AddMember("Animations", animClipsValue, alloc);

	//
	//���� ���� ����
	//
	FILE* animAsset;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);    // Accept() traverses the DOM and generates Handler events.

	std::string animFileName = path_name + "_Animation.asset";
	fopen_s(&animAsset, animFileName.c_str(), "wt");

	if (animAsset != nullptr) {
		fputs(sb.GetString(), animAsset);
		fclose(animAsset);
	}
}

void ResourceManager::ImportGeometryAsset(const std::filesystem::path& filePath)
{

	//읽기전에 현재 Geo가 존재하는지 체크
	bool isHas = false;
	for (const auto& e : mStaticGeometries) {
		if (e->Name.compare(filePath.string()) == 0) {
			isHas = true;
			break;
		}
	}
	if (isHas)
		return;

	isHas = false;
	for (const auto& e : mSkeletalGeometries) {
		if (e->Name.compare(filePath.string()) == 0) {
			isHas = true;
			break;
		}
	}
	if (isHas)
		return;


	FILE* meshAssetFile;
	fopen_s(&meshAssetFile, filePath.string().c_str(), "rb");

	if (meshAssetFile == nullptr) {
		throw exception("Not File!!");
		return;
	}

	//
	//파일 읽기
	//
	fseek(meshAssetFile, 0, SEEK_END);
	int nFileSize = ftell(meshAssetFile);
	fseek(meshAssetFile, 0, SEEK_SET);

	char* buf = new char[nFileSize];	//파일크기만큼 버퍼생성

	FileReadStream is(meshAssetFile, buf, nFileSize);	//스트림 생성

	Document doc;
	ParseResult ok = doc.ParseStream(is);	//스트림에서 JSON형식으로 파싱

	fclose(meshAssetFile);	//파일 닫기
	delete[] buf;

	if (!ok) {	//파싱에 실패했다면
		char str[256] = { 0, };
		sprintf_s(str, "JSON parse error: %s %d \n", GetParseError_En(ok.Code()), (int)ok.Offset());
		throw exception(str);
		return;
	}

	assert(doc.IsObject());

	//
	//Type 확인
	//
	assert(doc.HasMember("Type"));
	assert(doc["Type"].IsString());
	if ((strcmp(doc["Type"].GetString(), "StaticGeometry") != 0) || (strcmp(doc["Type"].GetString(), "SkeletalGeometry") != 0)) {
		throw exception("Asset Type Error!");
		return;
	}

	//
	//Type에 따라서 Static인지 Skeletal인지 체크
	//
	Geometry* tempGeo = nullptr;
	if (strcmp(doc["Type"].GetString(), "StaticGeometry") == 0) {
		tempGeo = new StaticGeometry();
		tempGeo->IsStatic = true;
	}
	else if (strcmp(doc["Type"].GetString(), "SkeletalGeometry") == 0) {
		tempGeo = new SkeletalGeometry();
		tempGeo->IsStatic = false;
	}

	assert(tempGeo != nullptr);

	//
	//Name 확인
	//
	assert(doc.HasMember("Name"));
	assert(doc["Name"].IsString());
	tempGeo->Name = doc["Name"].GetString();


	//
	//Vertex 로드
	//
	assert(doc.HasMember("VertexFile"));
	assert(doc["VertexFile"].IsString());
	std::filesystem::path vertexName = doc["VertexFile"].GetString();

	FILE* vertexFile;
	fopen_s(&vertexFile, vertexName.string().c_str(), "rb");
	if (vertexFile == nullptr) {
		doc.RemoveAllMembers();
		throw exception("Not Have VertexFile!!! ");
		return;
	}

	//메쉬데이터 읽기
	MeshData* tempMesh = nullptr;
	if (tempGeo->IsStatic) {
		tempMesh = new StaticMeshData();

		while (true) {
			Vertex tempVtx;

			fread(&tempVtx, sizeof(Vertex), 1, vertexFile);
			if (feof(vertexFile))
				break;

			((StaticMeshData*)tempMesh)->Vertices.emplace_back(tempVtx);
		}
	}
	else if (!tempGeo->IsStatic) {
		tempMesh = new SkinnedMeshData();

		while (true) {
			SkinnedVertex tempVtx;

			fread(&tempVtx, sizeof(SkinnedVertex), 1, vertexFile);
			if (feof(vertexFile))
				break;

			((SkinnedMeshData*)tempMesh)->Vertices.emplace_back(tempVtx);
		}
	}

	fclose(vertexFile);


	//
	//Index 파일 로드
	//
	assert(doc.HasMember("IndexFile"));
	assert(doc["IndexFile"].IsString());
	std::string indexName = doc["IndexFile"].GetString();

	FILE* indexFile;
	fopen_s(&indexFile, indexName.c_str(), "rb");
	if (indexFile == nullptr) {
		doc.RemoveAllMembers();
		throw exception("Not Have IndexFile!!! ");
		return;
	}

	while (true) {
		uint16_t Index;
		fread(&Index, sizeof(uint16_t), 1, indexFile);
		if (feof(indexFile))
			break;

		tempMesh->Indices32.emplace_back(Index);
	}
	fclose(indexFile);


	if (tempGeo->IsStatic) {
		((StaticGeometry*)tempGeo)->Mesh = *static_cast<StaticMeshData*>(tempMesh);

	}
	else if (!tempGeo->IsStatic) {
		((SkeletalGeometry*)tempGeo)->Mesh = *static_cast<SkinnedMeshData*>(tempMesh);
	}

	//
	//스켈레톤 데이터 읽기
	//수정필요
	/*
	if (!tempGeo->IsStatic) {
		assert(doc.HasMember("SkeletonAsset"));
		assert(doc["SkeletonAsset"].IsString());
		std::string skeletonName = doc["SkeletonAsset"].GetString();
		if (mSkeletons.size() > 0)
			((SkeletalGeometry*)tempGeo)->Skeleton = mSkeletons[mSkeletons.size() - 1].get();
	}
	*/
	//
	//Submesh 확인
	//
	assert(doc.HasMember("SubMeshes"));
	assert(doc["SubMeshes"].IsArray());
	for (const auto& e : doc["SubMeshes"].GetArray()) {
		SubmeshGeometry tempSub;

		auto submesh = e.GetObjectW();

		//SubmeshName
		assert(submesh.HasMember("SubmeshName"));
		assert(submesh["SubmeshName"].IsString());
		tempSub.Name = submesh["SubmeshName"].GetString();

		//MaterialName
		assert(submesh.HasMember("MaterialName"));
		assert(submesh["MaterialName"].IsString());
		tempSub.MatName = submesh["MaterialName"].GetString();

		//
		//메테리얼 생성
		//
		std::filesystem::path matAssetFilePath = filePath.parent_path() / (tempSub.MatName + ".asset");
		//BuildMaterial(matAssetFilePath);
		ImportMaterialAsset(matAssetFilePath);


		//StartIndex
		assert(submesh.HasMember("StartIndex"));
		assert(submesh["StartIndex"].IsUint());
		tempSub.StartIndexLocation = submesh["StartIndex"].GetUint();

		//IndexCount
		assert(submesh.HasMember("IndexCount"));
		assert(submesh["IndexCount"].IsUint());
		tempSub.IndexCount = submesh["IndexCount"].GetUint();

		//BaseVertex
		assert(submesh.HasMember("BaseVertex"));
		assert(submesh["BaseVertex"].IsUint());
		tempSub.BaseVertexLocation = submesh["BaseVertex"].GetUint();

		tempGeo->SubGeoInfos.push_back(move(tempSub));
	}

	if (tempGeo->IsStatic) {
		mStaticGeometries.push_back(make_unique<StaticGeometry>(*(StaticGeometry*)(tempGeo)));
	}
	else if (!tempGeo->IsStatic) {
		mSkeletalGeometries.push_back(make_unique<SkeletalGeometry>(*(SkeletalGeometry*)(tempGeo)));
	}

	//메모리릭 예방
	doc.RemoveAllMembers();
}

void ResourceManager::ImportMaterialAsset(const std::filesystem::path& filePath)
{
	//에셋 추가전에 현재 메모리에 있는지 체크
	for (const auto& e : mMaterials) {
		if (e->Name.compare(filePath.string()) == 0) {
			return;
		}
	}

	FILE* matAssetFile = nullptr;
	fopen_s(&matAssetFile, filePath.string().c_str(), "rb");
	if (matAssetFile == nullptr) {
		throw exception("Not have Mat Asstet");
		return;
	}

	//
	//에셋파일 읽기
	//
	fseek(matAssetFile, 0, SEEK_END);
	int nFileSize = ftell(matAssetFile);
	fseek(matAssetFile, 0, SEEK_SET);

	char* buf = new char[nFileSize];	//버퍼 생성

	FileReadStream is(matAssetFile, buf, nFileSize);	//스트림 생성

	Document doc;
	ParseResult ok = doc.ParseStream(is);	//스트림에서 JSON파싱

	//뒷정리 파일닫기, 버퍼 할당 해제
	fclose(matAssetFile);
	delete[] buf;

	if (!ok) {	//파싱에 실패했다면
		char str[256] = { 0, };
		sprintf_s(str, "JSON parse error: %s %d \n", GetParseError_En(ok.Code()), (int)ok.Offset());
		throw exception(str);
		return;
	}

	assert(doc.IsObject());

	//
	//Type 확인
	//
	assert(doc.HasMember("Type"));
	assert(doc["Type"].IsString());
	if (strcmp(doc["Type"].GetString(), "Material") != 0) {	//MeshAsset이 아닌 경우
		throw exception("Asset Type Error!");
		return;
	}

	std::unique_ptr<Material> tempMat = std::make_unique<Material>();

	//
	//Name 확인
	//
	assert(doc.HasMember("Name"));
	assert(doc["Name"].IsString());
	tempMat->Name = doc["Name"].GetString();

	//
	//Texture 확인
	//
	assert(doc.HasMember("Textures"));
	assert(doc["Textures"].IsObject());
	auto textures = doc["Textures"].GetObjectW();

	//diffuse 텍스쳐
	if (textures.HasMember("diffuse")) {	
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["diffuse"].GetString();
		tempTexture->isDirty = false;

		ImportTextureAsset(tempTexture->Name);

		tempMat->TexName[(int)TextureType::DIFFUSE] = textures["diffuse"].GetString();

		bool HasTex = false;
		for (auto& e : mTextures) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::DIFFUSE]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			mTextures.push_back(move(tempTexture));
	}

	//normal 텍스쳐
	if (textures.HasMember("normal")) {	
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["normal"].GetString();
		tempTexture->isDirty = false;
		tempMat->TexName[(int)TextureType::NORMAL] = textures["normal"].GetString();

		ImportTextureAsset(tempTexture->Name);

		bool HasTex = false;
		for (auto& e : mTextures) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::NORMAL]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			mTextures.push_back(move(tempTexture));
	}

	//specular 텍스쳐
	if (textures.HasMember("specular")) {	
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["specular"].GetString();
		tempTexture->isDirty = false;
		tempMat->TexName[(int)TextureType::SPECULAR] = textures["specular"].GetString();

		ImportTextureAsset(tempTexture->Name);

		bool HasTex = false;
		for (auto& e : mTextures) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::SPECULAR]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			mTextures.push_back(move(tempTexture));
	}

	//
	//DiffuseAlbedo 확인
	//
	assert(doc.HasMember("DiffuseAlbedo"));
	assert(doc["DiffuseAlbedo"].IsArray());

	auto albedo = doc["DiffuseAlbedo"].GetArray();
	float* albedoIt = (float*)&tempMat->DiffuseAlbedo;
	for (int i = 0; i < (int)albedo.Size(); ++i) {
		*(float*)(albedoIt + i) = albedo[i].GetFloat();
	}

	//
	//FresnelR0 확인
	//
	assert(doc.HasMember("FresnelR0"));
	assert(doc["FresnelR0"].IsArray());

	auto fresnel = doc["FresnelR0"].GetArray();
	float* fresnelIt = (float*)&tempMat->FresnelR0;
	for (int i = 0; i < (int)fresnel.Size(); ++i) {
		*(float*)(fresnelIt + i) = fresnel[i].GetFloat();
	}

	//
	//Roughness 확인
	//
	assert(doc.HasMember("Roughness"));
	assert(doc["Roughness"].IsFloat());
	tempMat->Roughness = doc["Roughness"].GetFloat();


	//
	//MatTransform 확인
	//
	assert(doc.HasMember("MatTransform"));
	assert(doc["MatTransform"].IsArray());
	auto matTr = doc["MatTransform"].GetArray();
	float* matIt = (float*)&tempMat->MatTransform;
	for (int i = 0; i < (int)matTr.Size(); ++i) {
		*(float*)(matIt + i) = matTr[i].GetFloat();
	}

	//메테리얼 설정
	tempMat->isDirty = true;

	mMaterials.push_back(move(tempMat));

	doc.RemoveAllMembers();
}

void ResourceManager::ImportTextureAsset(const std::filesystem::path& filePath)
{
	//메모리에 이미 존재하는 경우 추가 패쓰
	for (const auto& e : mTextures) {
		if (e->Name.compare(filePath.string()) == 0) {	
			return;
		}
	}

	std::unique_ptr<Texture> tempTex = std::make_unique<Texture>();
	tempTex->Name = filePath.is_absolute() ? filePath.relative_path().string() : filePath.string();

	mTextures.push_back(move(tempTex));
}

void ResourceManager::ImportSkeletonAsset(std::string& fileName)
{
	std::string path = fileName.substr(0, fileName.find_last_of('\\'));
	std::string path_name = fileName.substr(0, fileName.find_last_of('.'));

	std::string skeletonAssetPath = path_name + "_Skeleton.asset";

	FILE* skeltonAssetFile;
	fopen_s(&skeltonAssetFile, skeletonAssetPath.c_str(), "rb");

	if (skeltonAssetFile == nullptr) {
		return;
	}

	//
	//���� ũ�� �˾Ƴ���
	//
	fseek(skeltonAssetFile, 0, SEEK_END);
	int nFileSize = ftell(skeltonAssetFile);
	fseek(skeltonAssetFile, 0, SEEK_SET);

	char* buf = new char[nFileSize];	//���� ũ�⿡ ���� ���� ����

	FileReadStream is(skeltonAssetFile, buf, nFileSize);	//��Ʈ�� ����

	Document doc;
	ParseResult ok = doc.ParseStream(is);	//��Ʈ������ JSON�Ľ�

	fclose(skeltonAssetFile);	//���� �ݱ�

	if (!ok) {	//�Ľ��� �����ߴٸ�
		std::cout << "JSON parse error: " << GetParseError_En(ok.Code()) << ok.Offset() << std::endl;
	}

	assert(doc.IsObject());

	std::unique_ptr<Skeleton> tempSkeleton = std::make_unique<Skeleton>();

	//
	//Type ��������
	//
	assert(doc.HasMember("Type"));
	assert(doc["Type"].IsString());
	if (strcmp(doc["Type"].GetString(), "Skeleton") != 0) {	//Skeleton Asset�� �ƴѰ��
		OutputDebugStringA(doc["Type"].GetString());
		//���� ó��
	}

	//
	//����Ʈ ��������
	//
	assert(doc.HasMember("Joints"));
	assert(doc["Joints"].IsArray());

	for (int i = 0; i < (int)doc["Joints"].GetArray().Size(); ++i) {

		auto joint = doc["Joints"].GetArray()[i].GetObjectW();	//���� �迭��ҿ��� ����޽� ������Ʈ�� ������
		Joint jnt;

		//ParentIndex
		assert(joint.HasMember("ParentIndex"));
		assert(joint["ParentIndex"].IsInt());
		jnt.mParentIndex = joint["ParentIndex"].GetInt();

		//Name
		assert(joint.HasMember("Name"));
		assert(joint["Name"].IsString());
		jnt.mName = joint["Name"].GetString();


		//Bind-Transform
		assert(joint.HasMember("BindTime-Transform"));
		assert(joint["BindTime-Transform"].IsArray());

		auto transformArr = joint["BindTime-Transform"].GetArray();
		float* matIt = (float*)&jnt.mGlobalBindposeInverse;

		for (int j = 0; j < (int)transformArr.Size(); ++j) {
			*(matIt + j) = transformArr[j].GetFloat();
		}

		tempSkeleton->mJoints.emplace_back(jnt);
	}

	mSkeletons.emplace_back(move(tempSkeleton));

	//�޸� �� �߻� ����
	doc.RemoveAllMembers();

}

void ResourceManager::ImportAnimationAsset(std::string& fileName)
{
	std::string path = fileName.substr(0, fileName.find_last_of('\\'));
	std::string path_name = fileName.substr(0, fileName.find_last_of('.'));

	std::string animationFilePath = path_name + "_Animation.asset";

	FILE* animAssetFile;
	fopen_s(&animAssetFile, animationFilePath.c_str(), "rb");

	if (animAssetFile == nullptr) {
		return;
	}

	//
	//���� ũ�� �˾Ƴ���
	//
	fseek(animAssetFile, 0, SEEK_END);
	int nFileSize = ftell(animAssetFile);
	fseek(animAssetFile, 0, SEEK_SET);

	char* buf = new char[nFileSize];	//���� ũ�⿡ ���� ���� ����

	FileReadStream is(animAssetFile, buf, nFileSize);	//��Ʈ�� ����

	Document doc;
	ParseResult ok = doc.ParseStream(is);	//��Ʈ������ JSON�Ľ�

	fclose(animAssetFile);	//���� �ݱ�

	if (!ok) {	//�Ľ��� �����ߴٸ�
		std::cout << "JSON parse error: " << GetParseError_En(ok.Code()) << ok.Offset() << std::endl;
	}

	assert(doc.IsObject());

	std::unique_ptr<AnimationNameClip> tempAnim = std::make_unique<AnimationNameClip>();

	//
	//Type ��������
	//
	assert(doc.HasMember("Type"));
	assert(doc["Type"].IsString());
	if (strcmp(doc["Type"].GetString(), "Animation") != 0) {	//Animation Asset�� �ƴѰ��
		OutputDebugStringA(doc["Type"].GetString());
		//���� ó��
	}


	//
	//�ش� ���̷��� ����
	//
	assert(doc.HasMember("SkeletonAsset"));
	assert(doc["SkeletonAsset"].IsString());
	auto skeletonFileName = doc["SkeletonAsset"].GetString();


	//
	//Ŭ�� ��������
	//
	assert(doc.HasMember("Animations"));
	assert(doc["Animations"].IsArray());

	auto animClips = doc["Animations"].GetArray();

	for (int i = 0; i < (int)animClips.Size(); ++i) {
		auto curClip = animClips[i].GetObjectW();

		//Ŭ�� �̸�
		assert(curClip.HasMember("Name"));
		assert(curClip["Name"].IsString());
		std::string clipName = curClip["Name"].GetString();

		//Ŭ�� ������
		assert(curClip.HasMember("ClipData"));
		assert(curClip["ClipData"].IsArray());

		AnimationClip animClip;
		if (mSkeletons.size() > 0) {
			animClip.Skeleton = mSkeletons[mSkeletons.size() - 1].get();	//���� �ֱٿ� ���� ���븦 �޾ƿ�
			animClip.Skeleton->IsAnimationable = true;
		}

		for (int j = 0; j < (int)curClip["ClipData"].Size(); ++j) {
			//������ �迭
			assert(curClip["ClipData"][j].IsArray());
			auto bones = curClip["ClipData"][j].GetArray();
			BoneAnimation boneAnim;

			//�� ���� Ű�����ӵ��� �迭
			for (int k = 0; k < (int)bones.Size(); ++k) {
				assert(bones[k].IsObject());
				auto curKeyframe = bones[k].GetObjectW();

				Keyframe keyFrame;
				//TimePos
				assert(curKeyframe.HasMember("TimePos"));
				assert(curKeyframe["TimePos"].IsFloat());
				keyFrame.TimePos = curKeyframe["TimePos"].GetFloat();

				//Translation
				assert(curKeyframe.HasMember("Translation"));
				assert(curKeyframe["Translation"].IsArray());

				auto translationArr = curKeyframe["Translation"].GetArray();
				float* trIt = (float*)&keyFrame.Translation;

				for (int idx = 0; idx < (int)translationArr.Size(); ++idx) {
					*(trIt + idx) = translationArr[idx].GetFloat();
				}


				//Quaternion
				assert(curKeyframe.HasMember("Quaternion"));
				assert(curKeyframe["Quaternion"].IsArray());

				auto quatArr = curKeyframe["Quaternion"].GetArray();
				float* quatIt = (float*)&keyFrame.RotationQuat;

				for (int idx = 0; idx < (int)quatArr.Size(); ++idx) {
					*(quatIt + idx) = quatArr[idx].GetFloat();
				}

				//Scale
				assert(curKeyframe.HasMember("Scale"));
				assert(curKeyframe["Scale"].IsArray());

				auto scaleArr = curKeyframe["Scale"].GetArray();
				float* sclIt = (float*)&keyFrame.Scale;

				for (int idx = 0; idx < (int)scaleArr.Size(); ++idx) {
					*(sclIt + idx) = scaleArr[idx].GetFloat();
				}

				boneAnim.Keyframes.emplace_back(keyFrame);
			}

			animClip.BoneAnimations.emplace_back(boneAnim);
		}

		tempAnim->insert(std::pair<std::string, AnimationClip>(clipName, animClip));

	}

	mAnimations = move(tempAnim);

	//�޸� �� �߻� ����
	doc.RemoveAllMembers();
}

void ResourceManager::ExportGeometryAsset(const std::string& geoName)
{
	std::filesystem::path filePath(geoName);
	
	Geometry* geo = nullptr;
	for (const auto& e : mStaticGeometries) {
		if (e->Name.compare(geoName) == 0) {
			geo = e.get();
			break;
		}
	}
	if (geo == nullptr) {
		for (const auto& e : mSkeletalGeometries) {
			if (e->Name.compare(geoName) == 0) {
				geo = e.get();
				break;
			}
		}
	}
	if (geo == nullptr) {
		throw exception("Can't Find Geometry");
		return;
	}
	assert(geo != nullptr);

	//깨끗한 경우 저장할 필요없음
	if (!geo->isDirty)
		return;

	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//타입 생성
	//
	if (geo->IsStatic) {
		doc.AddMember("Type", "StaticGeometry", alloc);
	}
	else {
		doc.AddMember("Type", "SkeletalGeometry", alloc);
	}


	//
	//이름 생성
	//
	Value nameValue(geo->Name.c_str(), (rapidjson::SizeType)geo->Name.length(), alloc);
	doc.AddMember("Name", nameValue, alloc);

	//
	//정점 bin 생성
	//
	std::filesystem::path VertexDataFileName = (filePath.parent_path() / "MeshData" / (filePath.stem().string() + std::string("_vtx.bin")));
	FILE* vertexFile;
	fopen_s(&vertexFile, VertexDataFileName.string().c_str(), "wb");

	assert(vertexFile != nullptr);
	if (vertexFile != nullptr) {

		//정점파일 생성
		if (geo->IsStatic) {
			for (int i = 0; i < (int)((StaticGeometry*)geo)->Mesh.Vertices.size(); ++i) {
				auto& vertexData = ((StaticGeometry*)geo)->Mesh.Vertices[i];
				int vertexSize = sizeof(vertexData);

				fwrite(&vertexData, vertexSize, 1, vertexFile);
			}
		}
		else {
			for (int i = 0; i < (int)((SkeletalGeometry*)geo)->Mesh.Vertices.size(); ++i) {
				auto& vertexData = ((SkeletalGeometry*)geo)->Mesh.Vertices[i];
				int vertexSize = sizeof(vertexData);

				fwrite(&vertexData, vertexSize, 1, vertexFile);
			}
		}

		fclose(vertexFile);
	}
	else {
		throw exception("Can't Create VertexFile");
		return;
	}

	//
	//인덱스 bin 생성
	//
	std::filesystem::path IndexDataFileName = (filePath.parent_path() / "MeshData" / (filePath.stem().string() + std::string("_idx.bin")));
	FILE* indexFile;
	fopen_s(&indexFile, IndexDataFileName.string().c_str(), "wb");

	assert(indexFile != nullptr);
	if (indexFile != nullptr) {

		//����ƽ ������ ���
		if (geo->IsStatic) {
			auto& indexData = ((StaticGeometry*)geo)->Mesh.GetIndices16();
			for (int i = 0; i < (int)indexData.size(); ++i) {
				int indexSize = sizeof(uint16_t);
				fwrite(&indexData[i], indexSize, 1, indexFile);
			}
		}
		else {
			auto& indexData = ((SkeletalGeometry*)geo)->Mesh.GetIndices16();
			for (int i = 0; i < (int)indexData.size(); ++i) {
				int indexSize = sizeof(uint16_t);
				fwrite(&indexData[i], indexSize, 1, indexFile);
			}
		}

		fclose(indexFile);
	}
	else {
		throw exception("Can't Create InexFile");
		return;
	}

	rapidjson::Value vertexValue(VertexDataFileName.string().c_str(), static_cast<SizeType>(VertexDataFileName.string().length()), alloc);
	rapidjson::Value indexValue(IndexDataFileName.string().c_str(), static_cast<SizeType>(IndexDataFileName.string().length()), alloc);

	doc.AddMember("VertexFile", vertexValue, alloc);
	doc.AddMember("IndexFile", indexValue, alloc);

	//
	//스켈레톤 에셋
	//
	/*
	if (!geo->IsStatic) {	//���̷�Ż�� ���
		auto skelGeo = (SkeletalGeometry*)geo;
		auto skeletonFileName = (filePath.parent_path() / (filePath.stem().string() + "_Skeleton.asset"));
		Value nameValue(skeletonFileName.string().c_str(), (rapidjson::SizeType)skeletonFileName.string().length(), alloc);
		doc.AddMember("SkeletonAsset", nameValue, alloc);
	}
	*/

	//
	//서브지오 생성
	//
	Value subMeshesArrayValue(kArrayType);
	for (auto& sub : geo->SubGeoInfos) {
		Value subMeshValue(kArrayType);
		subMeshValue.SetObject();

		rapidjson::Value nameValue;
		nameValue.SetString(sub.Name.c_str(), static_cast<SizeType>(sub.Name.length()), alloc);

		rapidjson::Value matNameValue;
		matNameValue.SetString(sub.MatName.c_str(), static_cast<SizeType>(sub.MatName.length()), alloc);

		ExportMaterialAsset(sub.MatName);

		rapidjson::Value startIndexValue;
		startIndexValue.SetUint(sub.StartIndexLocation);

		rapidjson::Value indexCountValue;
		indexCountValue.SetUint(sub.IndexCount);

		rapidjson::Value baseVetexValue;
		baseVetexValue.SetUint(sub.BaseVertexLocation);


		subMeshValue.AddMember("SubmeshName", nameValue, alloc);
		subMeshValue.AddMember("MaterialName", matNameValue, alloc);
		subMeshValue.AddMember("StartIndex", startIndexValue, alloc);
		subMeshValue.AddMember("IndexCount", indexCountValue, alloc);
		subMeshValue.AddMember("BaseVertex", baseVetexValue, alloc);

		subMeshesArrayValue.PushBack(subMeshValue, alloc);
	}

	doc.AddMember("SubMeshes", subMeshesArrayValue, alloc);


	//
	//asset 파일 생성
	//
	FILE* assetFile;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);
	fopen_s(&assetFile, filePath.string().c_str(), "wt");

	if (assetFile != nullptr) {
		fputs(sb.GetString(), assetFile);
		fclose(assetFile);
	}

	doc.RemoveAllMembers();
}

void ResourceManager::ExportMaterialAsset(const std::string& matName)
{
	std::filesystem::path filePath(matName);

	Material* mat = nullptr;

	for (const auto& e : mMaterials) {
		if (e->Name.compare(matName) == 0) {
			mat = e.get();
			break;
		}
	}
	assert(mat != nullptr);

	//깨끗한 경우 저장할 필요없음
	if (!mat->isDirty)
		return;

	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//타입 생성
	//
	doc.AddMember("Type", "Material", alloc);

	//
	//이름 생성
	//
	Value nameValue(mat->Name.c_str(), (rapidjson::SizeType)mat->Name.size(), alloc);
	doc.AddMember("Name", nameValue, alloc);

	//
	//텍스쳐 생성
	//
	Value texArrayValue(kArrayType);
	texArrayValue.SetObject();
	for (int texIdx = 0; texIdx < (int)TextureType::NUM; ++texIdx) {	
		if (mat->TexName[texIdx].empty())
			continue;

		std::string texType;

		switch (texIdx) {
		case (int)TextureType::DIFFUSE:
			texType = "diffuse";
			break;
		case (int)TextureType::NORMAL:
			texType = "normal";
			break;
		case (int)TextureType::SPECULAR:
			texType = "specular";
			break;
		case (int)TextureType::EMISSIVE:
			texType = "emissive";
			break;
		default:
			texType = "";
			break;
		}

		//텍스쳐 파일 값 생성
		Value arrKey(texType.c_str(), (rapidjson::SizeType)texType.size(), alloc);
		Value arrValue(mat->TexName[texIdx].c_str(), (rapidjson::SizeType)mat->TexName[texIdx].size(), alloc);
		texArrayValue.AddMember(arrKey, arrValue, alloc);
	}

	doc.AddMember("Textures", texArrayValue, alloc);

	//
	//DiffuseAlbedo
	//
	Value diffuseAlbedoValue(kArrayType);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.x, alloc);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.y, alloc);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.z, alloc);
	diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.w, alloc);
	doc.AddMember("DiffuseAlbedo", diffuseAlbedoValue, alloc);

	//
	//FresnelR0
	//
	Value fresnelValue(kArrayType);
	fresnelValue.PushBack(mat->FresnelR0.x, alloc);
	fresnelValue.PushBack(mat->FresnelR0.y, alloc);
	fresnelValue.PushBack(mat->FresnelR0.z, alloc);
	doc.AddMember("FresnelR0", fresnelValue, alloc);

	//
	//Roughness
	//
	Value roughnessValue(mat->Roughness);
	doc.AddMember("Roughness", roughnessValue, alloc);

	//
	//MatTransform
	//
	Value matTrValue(kArrayType);
	float* it = (float*)&mat->MatTransform;
	for (uint32_t i = 0; i < 16; ++i) {
		Value matValue(*(it + i));
		matTrValue.PushBack(matValue, alloc);
	}
	doc.AddMember("MatTransform", matTrValue, alloc);

	//
	//에셋 파일 생성
	//
	FILE* matAsset;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);

	fopen_s(&matAsset, filePath.string().c_str(), "wt");

	if (matAsset != nullptr) {
		fputs(sb.GetString(), matAsset);
		fclose(matAsset);
	}

	doc.RemoveAllMembers();
}


int ResourceManager::FindMaterialIndexForName(std::string matName)
{
	int idx = -1;

	for (size_t i = 0; i < mMaterials.size(); ++i) {
		if (mMaterials[i]->Name.compare(matName) == 0) {
			idx = (int)i;
			break;
		}
	}

	return idx;
}


void ResourceManager::LoadMapAsset(rapidjson::Document& doc, ResourceManager&)
{
}

void ResourceManager::LoadGeometryAsset(rapidjson::Document& doc, ResourceManager& resource)
{
	//
	//�迭�� ����Ǿ� �ִ��� üũ
	//

	//
	//type�� ���� Static���� Skeletal���� ����
	//
	Geometry* tempGeo = nullptr;
	if (strcmp(doc["Type"].GetString(), "StaticGeometry") == 0) {
		tempGeo = new StaticGeometry();
		tempGeo->IsStatic = true;
	}
	else if (strcmp(doc["Type"].GetString(), "SkeletalGeometry") == 0) {
		tempGeo = new SkeletalGeometry();
		tempGeo->IsStatic = false;
	}

	assert(tempGeo != nullptr);

	//
	//Name ��������
	//
	assert(doc.HasMember("Name"));
	assert(doc["Name"].IsString());
	tempGeo->Name = doc["Name"].GetString();


	//
	//Vertex ��������
	//
	assert(doc.HasMember("VertexFile"));
	assert(doc["VertexFile"].IsString());
	std::string vertexName = doc["VertexFile"].GetString();

	FILE* vertexFile;
	fopen_s(&vertexFile, vertexName.c_str(), "rb");

	MeshData* tempMesh = nullptr;

	if (tempGeo->IsStatic) {
		tempMesh = new StaticMeshData();

		while (true) {
			Vertex tempVtx;

			fread(&tempVtx, sizeof(Vertex), 1, vertexFile);
			if (feof(vertexFile))
				break;

			((StaticMeshData*)tempMesh)->Vertices.emplace_back(tempVtx);
		}
	}
	else if (!tempGeo->IsStatic) {
		tempMesh = new SkinnedMeshData();

		while (true) {
			SkinnedVertex tempVtx;

			fread(&tempVtx, sizeof(SkinnedVertex), 1, vertexFile);
			if (feof(vertexFile))
				break;

			((SkinnedMeshData*)tempMesh)->Vertices.emplace_back(tempVtx);
		}
	}

	fclose(vertexFile);


	//
	//Index ��������
	//
	assert(doc.HasMember("IndexFile"));
	assert(doc["IndexFile"].IsString());
	std::string indexName = doc["IndexFile"].GetString();

	FILE* indexFile;
	fopen_s(&indexFile, indexName.c_str(), "rb");
	while (true) {
		uint16_t Index;
		fread(&Index, sizeof(uint16_t), 1, indexFile);
		if (feof(indexFile))
			break;

		tempMesh->Indices32.emplace_back(Index);
	}
	fclose(indexFile);


	if (tempGeo->IsStatic) {
		((StaticGeometry*)tempGeo)->Mesh = *static_cast<StaticMeshData*>(tempMesh);

	}
	else if (!tempGeo->IsStatic) {
		((SkeletalGeometry*)tempGeo)->Mesh = *static_cast<SkinnedMeshData*>(tempMesh);
	}

	//
	//���̷����� �ִ� ��� ��������
	//
	if (!tempGeo->IsStatic) {
		assert(doc.HasMember("SkeletonAsset"));
		assert(doc["SkeletonAsset"].IsString());
		std::string skeletonName = doc["SkeletonAsset"].GetString();
		if (resource.GetSkeletons().size() > 0)
			((SkeletalGeometry*)tempGeo)->Skeleton = resource.GetSkeletons()[resource.GetSkeletons().size() - 1].get();
	}

	//
	//Submesh ��������
	//
	assert(doc.HasMember("SubMeshes"));
	assert(doc["SubMeshes"].IsArray());
	for (int i = 0; i < (int)doc["SubMeshes"].GetArray().Size(); ++i) {
		SubmeshGeometry tempSub;

		auto submesh = doc["SubMeshes"].GetArray()[i].GetObjectW();	//���� �迭��ҿ��� ����޽� ������Ʈ�� ������

		//SubmeshName
		assert(submesh.HasMember("SubmeshName"));
		assert(submesh["SubmeshName"].IsString());
		tempSub.Name = submesh["SubmeshName"].GetString();

		//MaterialName
		assert(submesh.HasMember("MaterialName"));
		assert(submesh["MaterialName"].IsString());
		tempSub.MatName = submesh["MaterialName"].GetString();

		//
		//���׸��� ���� ��������
		//
		/*
		std::string path = fileName.substr(0, fileName.find_last_of('\\'));
		std::string matAssetFileName = path + "\\" + tempSub.MatName + ".asset";
		BuildMaterial(matAssetFileName);
		*/

		//StartIndex
		assert(submesh.HasMember("StartIndex"));
		assert(submesh["StartIndex"].IsUint());
		tempSub.StartIndexLocation = submesh["StartIndex"].GetUint();

		//IndexCount
		assert(submesh.HasMember("IndexCount"));
		assert(submesh["IndexCount"].IsUint());
		tempSub.IndexCount = submesh["IndexCount"].GetUint();

		//BaseVertex
		assert(submesh.HasMember("BaseVertex"));
		assert(submesh["BaseVertex"].IsUint());
		tempSub.BaseVertexLocation = submesh["BaseVertex"].GetUint();

		tempGeo->SubGeoInfos.emplace_back(move(tempSub));
	}


	if (tempGeo->IsStatic) {
		resource.GetStaticGeometries().emplace_back(make_unique<StaticGeometry>(*reinterpret_cast<StaticGeometry*>(tempGeo)));
	}
	else if (!tempGeo->IsStatic) {
		resource.GetSkeletalGeometries().emplace_back(make_unique<SkeletalGeometry>(*reinterpret_cast<SkeletalGeometry*>(tempGeo)));
	}

	//�޸� �� �߻� ����
	doc.RemoveAllMembers();
}

void ResourceManager::LoadTextureAsset(std::filesystem::path& path, ResourceManager& resource)
{
	for (size_t i = 0; i < resource.GetTextures().size(); ++i) {
		if (resource.GetTextures()[i]->Name.compare(path.string()) == 0) {	//�ε�� �ؽ��� �迭�� �̹� �ִ� ���
			return;
		}
	}

	std::unique_ptr<Texture> tempTex = std::make_unique<Texture>();
	tempTex->Name = path.is_absolute() ? path.string() : absolute(path).string();

	resource.GetTextures().emplace_back(move(tempTex));
}

void ResourceManager::LoadMaterialAsset(rapidjson::Document& doc, ResourceManager& resource)
{
	std::unique_ptr<Material> tempMat = std::make_unique<Material>();

	//
	//Name ��������
	//
	assert(doc.HasMember("Name"));
	assert(doc["Name"].IsString());
	tempMat->Name = doc["Name"].GetString();

	//
	//Texture ��������
	//
	assert(doc.HasMember("Textures"));
	assert(doc["Textures"].IsObject());
	auto textures = doc["Textures"].GetObjectW();

	if (textures.HasMember("diffuse")) {	//diffuse �ؽ��İ� �����ϸ� �ؽ��� ����, ���׸��� �� ����
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["diffuse"].GetString();
		tempMat->TexName[(int)TextureType::DIFFUSE] = textures["diffuse"].GetString();

		bool HasTex = false;
		for (auto& e : resource.GetTextures()) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::DIFFUSE]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			resource.GetTextures().emplace_back(move(tempTexture));
	}
	if (textures.HasMember("normal")) {	//normal �ؽ��İ� �����ϸ� �ؽ��� ����, ���׸��� �� ����
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["normal"].GetString();
		tempMat->TexName[(int)TextureType::NORMAL] = textures["normal"].GetString();

		bool HasTex = false;
		for (auto& e : resource.GetTextures()) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::NORMAL]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			resource.GetTextures().emplace_back(move(tempTexture));
	}
	if (textures.HasMember("specular")) {	//specular �ؽ��İ� �����ϸ� �ؽ��� ����, ���׸��� �� ����
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["specular"].GetString();
		tempMat->TexName[(int)TextureType::SPECULAR] = textures["specular"].GetString();
		resource.GetTextures().emplace_back(move(tempTexture));

		bool HasTex = false;
		for (auto& e : resource.GetTextures()) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::SPECULAR]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			resource.GetTextures().emplace_back(move(tempTexture));
	}

	//
	//DiffuseAlbedo ��������
	//
	assert(doc.HasMember("DiffuseAlbedo"));
	assert(doc["DiffuseAlbedo"].IsArray());

	auto albedo = doc["DiffuseAlbedo"].GetArray();
	float* albedoIt = (float*)&tempMat->DiffuseAlbedo;
	for (int i = 0; i < (int)albedo.Size(); ++i) {
		*(float*)(albedoIt + i) = albedo[i].GetFloat();
	}

	//
	//FresnelR0 ��������
	//
	assert(doc.HasMember("FresnelR0"));
	assert(doc["FresnelR0"].IsArray());

	auto fresnel = doc["FresnelR0"].GetArray();
	float* fresnelIt = (float*)&tempMat->FresnelR0;
	for (int i = 0; i < (int)fresnel.Size(); ++i) {
		*(float*)(fresnelIt + i) = fresnel[i].GetFloat();
	}

	//
	//Roughness ��������
	//
	assert(doc.HasMember("Roughness"));
	assert(doc["Roughness"].IsFloat());
	tempMat->Roughness = doc["Roughness"].GetFloat();


	//
	//MatTransform ��������
	//
	assert(doc.HasMember("MatTransform"));
	assert(doc["MatTransform"].IsArray());
	auto matTr = doc["MatTransform"].GetArray();
	float* matIt = (float*)&tempMat->MatTransform;
	for (int i = 0; i < (int)matTr.Size(); ++i) {
		*(float*)(matIt + i) = matTr[i].GetFloat();
	}

	//���� ����
	tempMat->isDirty = true;
	
	resource.GetMaterials().emplace_back(move(tempMat));

	doc.RemoveAllMembers();
}

void ResourceManager::LoadSkeletonAsset(rapidjson::Document& doc, ResourceManager& resource)
{

}

void ResourceManager::LoadAnimationAsset(rapidjson::Document& doc, ResourceManager& resource)
{

}

/*
void ResourceManager::ImportMapAsset(rapidjson::Document& doc, ResourceManager&)
{
}

void ResourceManager::ImportGeometryAsset(rapidjson::Document& doc, ResourceManager&)
{
}

void ResourceManager::ImportTextureAsset(std::filesystem::path& path, ResourceManager&)
{
}

void ResourceManager::ImportMaterialAsset(rapidjson::Document& doc, ResourceManager&)
{
}

void ResourceManager::ImportSkeletonAsset(rapidjson::Document& doc, ResourceManager&)
{
}

void ResourceManager::ImportAnimationAsset(rapidjson::Document& doc, ResourceManager&)
{
}
*/
void ResourceManager::CreateMaterialAsset(std::string& fileName, int geoIdx)
{

	if (!((size_t)geoIdx < mStaticGeometries.size() + mSkeletalGeometries.size()))
		return;

	std::string path = fileName.substr(0, fileName.find_last_of('\\'));
	std::string path_name = fileName.substr(0, fileName.find_last_of('.'));

	std::string textureFolder = path + "\\textures";	//���ο� �ؽ��İ� ����� ���

	int nResult = _access(textureFolder.c_str(), 0);	//�������� ������ ����
	if (nResult != 0) {
		CreateDirectoryA(textureFolder.c_str(), NULL);
	}

	Geometry* geo = nullptr;
	if ((size_t)geoIdx < mStaticGeometries.size()) {
		geo = mStaticGeometries[geoIdx].get();
		geo->IsStatic = true;
	}
	else {
		geo = mSkeletalGeometries[geoIdx - mStaticGeometries.size()].get();
		geo->IsStatic = false;
	}

	auto subMesh = geo->SubGeoInfos;
	//
	//���׸��� ���� ����
	//
	for (auto& sub : subMesh) {
		//���׸��� �˻�
		Material* mat = nullptr;
		for (auto& e : mMaterials) {
			if (e->Name.compare(sub.MatName) == 0) {
				mat = e.get();
				break;
			}
		}

		Document doc;
		doc.SetObject();
		auto& alloc = doc.GetAllocator();

		//
		//Ÿ�� ����
		//
		doc.AddMember("Type", "Material", alloc);

		//
		//�̸� ����
		//
		Value nameValue(mat->Name.c_str(), (rapidjson::SizeType)mat->Name.size(), alloc);
		doc.AddMember("Name", nameValue, alloc);

		//
		//�ؽ��� ��θ� ����
		//
		Value texArrayValue(kArrayType);
		texArrayValue.SetObject();
		for (int texIdx = 0; texIdx < (int)TextureType::NUM; ++texIdx) {	//�ؽ��� �迭�� ����ŭ
			if (mat->TexName[texIdx].empty())
				continue;

			for (int j = 0; j < (int)mTextures.size(); ++j) {	//�ؽ��� �迭�� �ؽ��İ� �����ϴ��� �˻�

				if (mTextures[j]->Name.compare(mat->TexName[texIdx]) == 0) {	//���� �̸��� �ؽ��İ� �����ϸ� ����
					std::string texType;
					switch (texIdx) {
					case (int)TextureType::DIFFUSE:
						texType = "diffuse";
						break;
					case (int)TextureType::NORMAL:
						texType = "normal";
						break;
					case (int)TextureType::SPECULAR:
						texType = "specular";
						break;
					case (int)TextureType::EMISSIVE:
						texType = "emissive";
						break;
					default:
						texType = "";
						break;
					}
					std::string texFileNamePath = mTextures[j]->Name;	//�ؽ��� ���
					int nameIdx = (int)texFileNamePath.find_last_of("\\");
					std::string texFileName = texFileNamePath.substr(nameIdx, texFileNamePath.size());

					regex r(R"((?:[^|*?\/:<>"])+)");
					auto beginIter = sregex_iterator(texFileName.begin(), texFileName.end(), r);
					auto endIter = sregex_iterator();
					int numIter = (int)std::distance(beginIter, endIter);

					for (std::sregex_iterator i = beginIter; i != endIter; ++i) {

						if (numIter == 1) {
							std::smatch match = *i;
							texFileName = match.str();
						}

						numIter--;
					}

					std::string newTexFileName = textureFolder + "\\" + texFileName;	//������ �ؽ��� ���

					//
					//���� ����
					//
					if (texFileNamePath.compare(newTexFileName) != 0) {	//���� �ؽ��� ���ϰ� ���ο� �ؽ��� ������ �ٸ� ��츸 ���� ����
						FILE* texFile, * newTexFile;
						fopen_s(&texFile, texFileNamePath.c_str(), "rb");
						fopen_s(&newTexFile, newTexFileName.c_str(), "wb");

						//
						//���۸� �̿��Ͽ� ����
						//
						char buf[1024];
						while (!feof(texFile))
						{
							fread(buf, sizeof(char), sizeof(buf), texFile);
							fwrite(buf, sizeof(char), sizeof(buf), newTexFile);
						}

						fflush(newTexFile);	//�÷���

						fclose(texFile);
						fclose(newTexFile);
					}

					//�ؽ��� Ÿ�Ժ��� ����
					Value arrKey(texType.c_str(), (rapidjson::SizeType)texType.size(), alloc);
					Value arrValue(newTexFileName.c_str(), (rapidjson::SizeType)newTexFileName.size(), alloc);
					texArrayValue.AddMember(arrKey, arrValue, alloc);

					break;
				}

			}

		}
		doc.AddMember("Textures", texArrayValue, alloc);

		//
		//DiffuseAlbedo
		//
		Value diffuseAlbedoValue(kArrayType);
		diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.x, alloc);
		diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.y, alloc);
		diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.z, alloc);
		diffuseAlbedoValue.PushBack(mat->DiffuseAlbedo.w, alloc);
		doc.AddMember("DiffuseAlbedo", diffuseAlbedoValue, alloc);

		//
		//FresnelR0
		//
		Value fresnelValue(kArrayType);
		fresnelValue.PushBack(mat->FresnelR0.x, alloc);
		fresnelValue.PushBack(mat->FresnelR0.y, alloc);
		fresnelValue.PushBack(mat->FresnelR0.z, alloc);
		doc.AddMember("FresnelR0", fresnelValue, alloc);

		//
		//Roughness
		//
		Value roughnessValue(mat->Roughness);
		doc.AddMember("Roughness", roughnessValue, alloc);

		//
		//MatTransform
		//
		Value matTrValue(kArrayType);
		float* it = (float*)&mat->MatTransform;
		for (uint32_t i = 0; i < 16; ++i) {
			Value matValue(*(it + i));
			matTrValue.PushBack(matValue, alloc);
		}
		doc.AddMember("MatTransform", matTrValue, alloc);


		//
		//���� ���� ����
		//
		FILE* matAsset;
		StringBuffer sb;
		PrettyWriter<StringBuffer> writer(sb);
		doc.Accept(writer);

		std::string matFileName = path + "\\" + mat->Name + ".asset";
		fopen_s(&matAsset, matFileName.c_str(), "wt");

		if (matAsset != nullptr) {
			fputs(sb.GetString(), matAsset);
			fclose(matAsset);
		}
	}

}

void ResourceManager::CreateGeometryAsset(std::string& fileName, int geoIdx)
{
	std::string path = fileName.substr(0, fileName.find_last_of('\\'));
	std::string path_name = fileName.substr(0, fileName.find_last_of('.'));

	Geometry* geo = nullptr;
	if (!((size_t)geoIdx < mStaticGeometries.size() + mSkeletalGeometries.size()))
		return;

	if ((size_t)geoIdx < mStaticGeometries.size()) {
		geo = mStaticGeometries[geoIdx].get();
		geo->IsStatic = true;
	}
	else {
		geo = mSkeletalGeometries[geoIdx - mStaticGeometries.size()].get();
		geo->IsStatic = false;
	}

	assert(geo != nullptr);

	Document doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	//
	//Ÿ�� ����
	//
	if (geo->IsStatic) {
		doc.AddMember("Type", "StaticGeometry", alloc);
	}
	else {
		doc.AddMember("Type", "SkeletalGeometry", alloc);
	}


	//
	//�̸� ����
	//
	Value nameValue(geo->Name.c_str(), (rapidjson::SizeType)geo->Name.length(), alloc);
	doc.AddMember("Name", nameValue, alloc);

	//
	//���� ������ bin ���� ����
	//
	std::string VertexDataFileName = path_name + std::string("_vtx.bin");
	FILE* vertexFile;
	fopen_s(&vertexFile, VertexDataFileName.c_str(), "wb");

	assert(vertexFile != nullptr);
	if (vertexFile != nullptr) {

		//����ƽ ������ ���
		if (geo->IsStatic) {
			for (int i = 0; i < (int)((StaticGeometry*)geo)->Mesh.Vertices.size(); ++i) {
				auto& vertexData = ((StaticGeometry*)geo)->Mesh.Vertices[i];
				int vertexSize = sizeof(vertexData);

				fwrite(&vertexData, vertexSize, 1, vertexFile);
			}
		}
		else {
			for (int i = 0; i < (int)((SkeletalGeometry*)geo)->Mesh.Vertices.size(); ++i) {
				auto& vertexData = ((SkeletalGeometry*)geo)->Mesh.Vertices[i];
				int vertexSize = sizeof(vertexData);

				fwrite(&vertexData, vertexSize, 1, vertexFile);
			}
		}

		fclose(vertexFile);
	}

	//
	//�ε��� ������ bin ����
	//
	std::string IndexDataFileName = path_name + std::string("_idx.bin");
	FILE* indexFile;
	fopen_s(&indexFile, IndexDataFileName.c_str(), "wb");

	assert(indexFile != nullptr);
	if (indexFile != nullptr) {

		//����ƽ ������ ���
		if (geo->IsStatic) {
			auto& indexData = ((StaticGeometry*)geo)->Mesh.GetIndices16();
			for (int i = 0; i < (int)indexData.size(); ++i) {
				int indexSize = sizeof(uint16_t);
				fwrite(&indexData[i], indexSize, 1, indexFile);
			}
		}
		else {
			auto& indexData = ((SkeletalGeometry*)geo)->Mesh.GetIndices16();
			for (int i = 0; i < (int)indexData.size(); ++i) {
				int indexSize = sizeof(uint16_t);
				fwrite(&indexData[i], indexSize, 1, indexFile);
			}
		}

		fclose(indexFile);
	}
	rapidjson::Value vertexValue(VertexDataFileName.c_str(), static_cast<SizeType>(VertexDataFileName.length()), alloc);
	rapidjson::Value indexValue(IndexDataFileName.c_str(), static_cast<SizeType>(IndexDataFileName.length()), alloc);

	doc.AddMember("VertexFile", vertexValue, alloc);
	doc.AddMember("IndexFile", indexValue, alloc);

	//
	//���̷�Ż ���� ����
	//
	if (!geo->IsStatic) {	//���̷�Ż�� ���
		auto skeletonFileName = path_name + "_Skeleton.asset";
		Value nameValue(skeletonFileName.c_str(), (rapidjson::SizeType)skeletonFileName.length(), alloc);
		doc.AddMember("SkeletonAsset", nameValue, alloc);
	}


	//
	//���� �޽� ����
	//
	Value subMeshesArrayValue(kArrayType);
	for (auto& sub : geo->SubGeoInfos) {
		Value subMeshValue(kArrayType);
		subMeshValue.SetObject();

		rapidjson::Value nameValue;
		nameValue.SetString(sub.Name.c_str(), static_cast<SizeType>(sub.Name.length()), alloc);

		rapidjson::Value matNameValue;
		matNameValue.SetString(sub.MatName.c_str(), static_cast<SizeType>(sub.MatName.length()), alloc);

		rapidjson::Value startIndexValue;
		startIndexValue.SetUint(sub.StartIndexLocation);

		rapidjson::Value indexCountValue;
		indexCountValue.SetUint(sub.IndexCount);

		rapidjson::Value baseVetexValue;
		baseVetexValue.SetUint(sub.BaseVertexLocation);


		subMeshValue.AddMember("SubmeshName", nameValue, alloc);
		subMeshValue.AddMember("MaterialName", matNameValue, alloc);
		subMeshValue.AddMember("StartIndex", startIndexValue, alloc);
		subMeshValue.AddMember("IndexCount", indexCountValue, alloc);
		subMeshValue.AddMember("BaseVertex", baseVetexValue, alloc);

		subMeshesArrayValue.PushBack(subMeshValue, alloc);
	}

	doc.AddMember("SubMeshes", subMeshesArrayValue, alloc);


	//
	//asset ���� ����
	//
	FILE* assetFile;
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	doc.Accept(writer);
	fopen_s(&assetFile, fileName.c_str(), "wt");

	if (assetFile != nullptr) {
		fputs(sb.GetString(), assetFile);
		fclose(assetFile);
	}

}

void ResourceManager::BuildMaterial(std::string& fileName)
{
	FILE* matAssetFile;
	fopen_s(&matAssetFile, fileName.c_str(), "rb");

	//
	//���� ũ�� �˾Ƴ���
	//
	fseek(matAssetFile, 0, SEEK_END);
	int nFileSize = ftell(matAssetFile);
	fseek(matAssetFile, 0, SEEK_SET);

	char* buf = new char[nFileSize];	//���� ũ�⿡ ���� ���� ����

	FileReadStream is(matAssetFile, buf, nFileSize);	//��Ʈ�� ����

	Document doc;
	ParseResult ok = doc.ParseStream(is);	//��Ʈ������ JSON�Ľ�

	fclose(matAssetFile);	//���� �ݱ�

	if (!ok) {	//�Ľ��� �����ߴٸ�
		std::cout << "JSON parse error: " << GetParseError_En(ok.Code()) << ok.Offset() << std::endl;
	}

	assert(doc.IsObject());

	//
	//Type ��������
	//
	assert(doc.HasMember("Type"));
	assert(doc["Type"].IsString());
	if (strcmp(doc["Type"].GetString(), "Material") != 0) {	//MeshAsset�� �ƴѰ��
		OutputDebugStringA(doc["Type"].GetString());
		//���� ó��
	}


	std::unique_ptr<Material> tempMat = std::make_unique<Material>();

	//
	//Name ��������
	//
	assert(doc.HasMember("Name"));
	assert(doc["Name"].IsString());
	tempMat->Name = doc["Name"].GetString();

	//
	//Texture ��������
	//
	assert(doc.HasMember("Textures"));
	assert(doc["Textures"].IsObject());
	auto textures = doc["Textures"].GetObjectW();

	if (textures.HasMember("diffuse")) {	//diffuse �ؽ��İ� �����ϸ� �ؽ��� ����, ���׸��� �� ����
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["diffuse"].GetString();
		tempMat->TexName[(int)TextureType::DIFFUSE] = textures["diffuse"].GetString();

		bool HasTex = false;
		for (auto& e : mTextures) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::DIFFUSE]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			mTextures.emplace_back(move(tempTexture));
	}
	if (textures.HasMember("normal")) {	//normal �ؽ��İ� �����ϸ� �ؽ��� ����, ���׸��� �� ����
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["normal"].GetString();
		tempMat->TexName[(int)TextureType::NORMAL] = textures["normal"].GetString();

		bool HasTex = false;
		for (auto& e : mTextures) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::NORMAL]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			mTextures.emplace_back(move(tempTexture));
	}
	if (textures.HasMember("specular")) {	//specular �ؽ��İ� �����ϸ� �ؽ��� ����, ���׸��� �� ����
		std::unique_ptr<Texture> tempTexture = std::make_unique<Texture>();
		tempTexture->Name = textures["specular"].GetString();
		tempMat->TexName[(int)TextureType::SPECULAR] = textures["specular"].GetString();
		mTextures.emplace_back(move(tempTexture));

		bool HasTex = false;
		for (auto& e : mTextures) {
			if (e->Name.compare(tempMat->TexName[(int)TextureType::SPECULAR]) == 0) {
				HasTex = true;
				break;
			}
		}
		if (!HasTex)	//�ؽ��� �迭�� ���� ���
			mTextures.emplace_back(move(tempTexture));
	}

	//
	//DiffuseAlbedo ��������
	//
	assert(doc.HasMember("DiffuseAlbedo"));
	assert(doc["DiffuseAlbedo"].IsArray());

	auto albedo = doc["DiffuseAlbedo"].GetArray();
	float* albedoIt = (float*)&tempMat->DiffuseAlbedo;
	for (int i = 0; i < (int)albedo.Size(); ++i) {
		*(float*)(albedoIt + i) = albedo[i].GetFloat();
	}

	//
	//FresnelR0 ��������
	//
	assert(doc.HasMember("FresnelR0"));
	assert(doc["FresnelR0"].IsArray());

	auto fresnel = doc["FresnelR0"].GetArray();
	float* fresnelIt = (float*)&tempMat->FresnelR0;
	for (int i = 0; i < (int)fresnel.Size(); ++i) {
		*(float*)(fresnelIt + i) = fresnel[i].GetFloat();
	}

	//
	//Roughness ��������
	//
	assert(doc.HasMember("Roughness"));
	assert(doc["Roughness"].IsFloat());
	tempMat->Roughness = doc["Roughness"].GetFloat();


	//
	//MatTransform ��������
	//
	assert(doc.HasMember("MatTransform"));
	assert(doc["MatTransform"].IsArray());
	auto matTr = doc["MatTransform"].GetArray();
	float* matIt = (float*)&tempMat->MatTransform;
	for (int i = 0; i < (int)matTr.Size(); ++i) {
		*(float*)(matIt + i) = matTr[i].GetFloat();
	}

	//���� ����
	tempMat->isDirty = true;

	mMaterials.emplace_back(move(tempMat));

	doc.RemoveAllMembers();
}

void ResourceManager::BuildDefaultGeometry()
{
	GeometryGenerator geoGen;
	StaticMeshData box;
	StaticMeshData sphere;
	StaticMeshData geosphere;
	StaticMeshData cylinder;
	StaticMeshData grid;
	StaticMeshData quad;

	std::vector<StaticMeshData> defaultShapes;
	std::vector<SubmeshGeometry> submeshs;

	//
	//�⺻���� �޽� ����
	//
	std::vector<std::string> shapeNames = { "Box", "Sphere", "Geosphere", "Cylinder", "Grid", "Quad" };

	box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	defaultShapes.emplace_back(box);


	sphere = geoGen.CreateSphere(1.0f, 20, 20);
	defaultShapes.emplace_back(sphere);


	geosphere = geoGen.CreateGeosphere(1.0f, 3);
	defaultShapes.emplace_back(geosphere);


	cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 5.0f, 20, 20);
	defaultShapes.emplace_back(cylinder);


	grid = geoGen.CreateGrid(30.0f, 40.0f, 60, 40);
	defaultShapes.emplace_back(grid);

	quad = geoGen.CreateQuad(0, 0, 10, 10, 1);
	defaultShapes.emplace_back(quad);

	//
	//����޽� ����
	//
	UINT VertexOffset = 0;
	UINT IndexOffset = 0;
	int nameIdx = 0;
	for (auto& e : defaultShapes) {
		SubmeshGeometry temp;
		temp.Name = shapeNames[nameIdx++];
		temp.MatName = "DefaultMat";
		temp.IndexCount = (uint32_t)e.Indices32.size();
		temp.BaseVertexLocation = VertexOffset;
		temp.StartIndexLocation = IndexOffset;

		VertexOffset += (UINT)e.Vertices.size();
		IndexOffset += (UINT)e.Indices32.size();

		submeshs.emplace_back(temp);
	}

	auto totalVertexCount = VertexOffset;

	std::vector<Vertex> vertices(totalVertexCount);

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	UINT j = 0;
	UINT k = 0;

	//
	//�ٿ�� �ڽ� ������ ���� ����
	//
	for (auto& e : defaultShapes) {
		auto& curSubmesh = submeshs[j++];
		for (size_t i = 0; i < e.Vertices.size(); ++i, ++k)
		{
			vertices[k].Position = e.Vertices[i].Position;
			vertices[k].Normal = e.Vertices[i].Normal;
			vertices[k].TexC = e.Vertices[i].TexC;
			vertices[k].TangentU = e.Vertices[i].TangentU;

			XMVECTOR P = XMLoadFloat3(&e.Vertices[i].Position);

			vMin = XMVectorMin(vMin, P);
			vMax = XMVectorMax(vMax, P);
		}

		XMStoreFloat3(&curSubmesh.Bounds.Center, 0.5f * (vMin + vMax));
		XMStoreFloat3(&curSubmesh.Bounds.Extents, 0.5f * (vMax - vMin));
	}

	//
	//�ε��� ����
	//
	std::vector<std::uint32_t> indices;
	for (auto& e : defaultShapes) {
		indices.insert(indices.end(), std::begin(e.Indices32), std::end(e.Indices32));
	}


	//
	//MeshData����
	//
	StaticMeshData mesh;
	mesh.Vertices = move(vertices);
	mesh.Indices32 = move(indices);

	//
	//Geometry����
	//
	auto geo = std::make_unique<StaticGeometry>();
	geo->Name = "DefaultShape";
	geo->Mesh = mesh;

	//
	//Submesh�̵�
	//
	geo->SubGeoInfos = submeshs;
	
	mStaticGeometries.emplace_back(move(geo));
}

void ResourceManager::ClearResource()
{
	//������Ʈ�� �ʱ�ȭ
	for (int i = 0; i < (int)mStaticGeometries.size(); ++i) {
		mStaticGeometries[i].reset();
	}
	mStaticGeometries.clear();

	for (int i = 0; i < (int)mSkeletalGeometries.size(); ++i) {
		mSkeletalGeometries[i].reset();
	}
	mSkeletalGeometries.clear();

	//�ؽ��� �ʱ�ȭ
	for (int i = 0; i < (int)mTextures.size(); ++i) {
		mTextures[i].reset();
	}
	mTextures.clear();

	//���׸��� �ʱ�ȭ
	for (int i = 0; i < (int)mMaterials.size(); ++i) {
		mMaterials[i].reset();
	}
	mMaterials.clear();

	//���� ������ �ʱ�ȭ
	for (int i = 0; i < (int)mSkeletons.size(); ++i) {
		mSkeletons[i].reset();
	}
	mSkeletons.clear();

	//�ִϸ��̼� ������ �ʱ�ȭ
	mAnimations.reset();

	/*
	//����Ʈ ���׸��� ����
	auto defaultMat = make_unique<Material>();
	defaultMat.get()->Name = "DefaultMat";
	mMaterials.emplace_back(move(defaultMat));

	//�⺻������ ����� ��� ����
	if (bCanUseDefaultShapes)
		BuildDefaultGeometry();
		*/
}