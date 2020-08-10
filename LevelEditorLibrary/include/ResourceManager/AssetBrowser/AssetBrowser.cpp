#include "AssetBrowser.h"

#include "../../Core/rapidjson/document.h"
#include "../../Core/rapidjson/error/en.h"
#include "../../Core/rapidjson/filereadstream.h"

#include <iostream>

using namespace std;
using namespace std::filesystem;
using namespace rapidjson;

AssetBrowser::AssetBrowser()
{
	RootPath = CurrentPath = ".\\Assets";

	if (!exists(CurrentPath)) {	//���� �н��� ���� ���丮�� ������
		cout << "Not Have Directory" << endl;
		create_directory(CurrentPath);	//���丮 ����
		cout << "Created Directory!" << endl;
	}
	else {
		cout << "Find Directory!" << endl << endl;;
	}

	//
	//CurrentPath로 이동
	//
	MoveDirectory(CurrentPath);

}

inline void ConvertToUpperCase(const std::string& str, std::string& converted)
{
	for (size_t i = 0; i < str.size(); ++i)
		converted += toupper(str[i]);
}


void AssetBrowser::MoveDirectory(std::filesystem::path& newPath)
{
	//
	//���� ��κ��� ���� ��η� ������ �ϴ� ��� ó��X
	//
	if (CurrentPath == RootPath && newPath.filename().compare("..") == 0) {
		SearchFileList(false);
		return;
	}

	///
	//�������� üũ
	//
	if (!newPath.has_extension()) {
		cout << "It's Directory" << endl;
	}
	else {
		cout << "It's Not Directory" << endl;
		return;
	}

	//
	//���� ��� �缳��
	//
	CurrentPath = newPath;

	//
	//���� ������ ��η� ���ϸ���Ʈ �����
	//
	SearchFileList();
}

void AssetBrowser::MoveDirectory(std::string& newDir)
{
	//
	//���� ��κ��� ���� ��η� ������ �ϴ� ��� ó��X
	//
	if (CurrentPath.compare(RootPath) == 0 && newDir.compare("..") == 0) {
		SearchFileList();
		return;
	}

	///
	//�������� üũ
	//
	bool HasDir = false;
	for (auto item : FileList) {
		if (item.FileName.compare(newDir) == 0) {	//������ ���ڿ��� �����̰� �����ϸ�
			if (item.FileType == FileType::Directory) {
				cout << "It's a Directory" << endl;
				HasDir = true;
				break;
			}
			else {
				cout << "It's Not a Directory" << endl;
				return;
			}
		}
	}

	//
	//���� ��� �缳��
	//
	if (HasDir) {
		CurrentPath = CurrentPath.string() + "\\" + newDir;
	}
	else {
		if (newDir.compare("..") == 0)
			CurrentPath = CurrentPath.parent_path();
		else {
			SearchFileList(false);
			return;
		}
	}

	//
	//���� ������ ��η� ���ϸ���Ʈ �����
	//
	SearchFileList();
}


void AssetBrowser::CreateNewMaterialAsset()
{

}

void AssetBrowser::CreateNewDiretory()
{

}

std::string AssetBrowser::GetDefaultMaterialFilename()
{
	for (int i = 0; i < 255; i++) {
		std::string defulatFilename = "DefaultAsset" + to_string(i) + ".asset";
		std::string newMatAssetPath = CurrentPath.string() + "\\" + defulatFilename;

		if (!exists(newMatAssetPath)) {	//������ �����ϴ��� üũ
			return defulatFilename;
		}
		else {
			continue;
		}

	}

	return "DefaultAsset.asset";
}

std::string AssetBrowser::GetAbsolutePathSelectedFile(std::string fileName)
{
	///
	//�������� üũ
	//
	for (auto item : FileList) {
		if (item.FileName.compare(fileName) == 0) {	//������ �̸��� ������ �����ϸ�
			if (item.FileType == FileType::AssetFile) {
				cout << "It's a File" << endl;
				return item.Path;
			}
			else if(item.FileType == FileType::Directory){
				break;
			}
		}
	}

	return nullptr;
}

std::string AssetBrowser::GetRelativePathSelectedFile(std::string fileName)
{
	///
	//�������� üũ
	//
	for (auto item : FileList) {
		if (item.FileName.compare(fileName) == 0) {	//������ �̸��� ������ �����ϸ�
			if (item.FileType == FileType::AssetFile) {
				cout << "It's a File" << endl;
				return (relative(CurrentPath).string() + "\\" + fileName);
			}
			else if (item.FileType == FileType::Directory) {
				break;
			}
		}
	}

	return nullptr;
}



void AssetBrowser::SearchFileList(bool saveList)
{
	//���ϸ�� �ʱ�ȭ
	FileList.clear();

	//���� ��ο� �ִ� �����̳� ���� �˻�
	auto pl = directory_iterator(CurrentPath);
	for (auto item : pl) {
		std::string fileType = "";
		std::string assetType = "";

		if (item.is_regular_file()) {
			std::string upperExt;

			ConvertToUpperCase(item.path().extension().string(), upperExt);

			if (upperExt.compare(".ASSET") != 0)	//AssetȮ���ڰ� �ƴϸ� �н�
				continue;

			//
			//�������� �ҷ�����
			//
			FILE* assetFile;
			fopen_s(&assetFile, item.path().string().c_str(), "rb");

			if (assetFile == nullptr) {
				cout << "Error - Can't Open AssetFile!" << endl << endl;
				continue;
			}

			//
			//���� ���� ũ�� �˾Ƴ���
			//
			fseek(assetFile, 0, SEEK_END);
			int nFileSize = ftell(assetFile);
			fseek(assetFile, 0, SEEK_SET);

			char* buf = new char[nFileSize];	//���� ũ�⿡ ���� ���� ����

			//
			//���� ���Ͽ��� json ���� �б�
			//
			FileReadStream is(assetFile, buf, nFileSize);	//��Ʈ�� ����

			Document doc;
			ParseResult ok = doc.ParseStream(is);	//��Ʈ������ JSON�Ľ�

			fclose(assetFile);	//���� �ݱ�

			if (!ok) {	//�Ľ��� �����ߴٸ�
				cout << "JSON parse error: " << GetParseError_En(ok.Code()) << ok.Offset() << std::endl;
			}

			//
			//Type ��������
			//
			assert(doc.HasMember("Type"));
			assert(doc["Type"].IsString());

			assetType = doc["Type"].GetString();

			//�޸� �� �߻� ����
			doc.RemoveAllMembers();
		}

		cout << "Name : " + item.path().filename().string() << endl;

		fileType = item.is_regular_file() ? "File" : "Directory";

		cout << "FileType : " + fileType << endl;

		cout << "AssetType : " + assetType << endl;

		cout << "Absolute Path : " + absolute(item.path()).string() << endl << endl;

		if (saveList) {
			FileInfo temp;
			temp.FileName = item.path().filename().string();
			temp.FileType = item.is_regular_file() ? FileType::AssetFile : FileType::Directory;
			temp.AssetType = assetType;
			temp.Path = absolute(item.path()).string();

			FileList.emplace_back(temp);
		}

	}
}
