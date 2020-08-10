#pragma once

#include <string>
#include <vector>
#include <filesystem> // C++17 standard header file name

/*FileEnum*/
enum class FileType : uint8_t {
	AssetFile,
	Directory
};

/*FileInfo*/
struct FileInfo {
	std::string FileName;
	FileType FileType;
	std::string AssetType;
	std::string Path;
};


/*Asset Browser*/
class AssetBrowser {
	//������
public:
	AssetBrowser();
	virtual ~AssetBrowser() = default;


	//�޼���
public:
	void MoveDirectory(std::filesystem::path& newPath);	//���ο� ��η� �̵�
	void MoveDirectory(std::string& newDir);	//���ο� ��η� �̵�

	void CreateNewMaterialAsset();	//���ο� ���׸��� ���� ������ �����Ѵ�
	void CreateNewDiretory();	//���ο� ���丮�� �����Ѵ�.
	
	std::string GetDefaultMaterialFilename();	//�⺻ ���׸��� ���ϸ� ����

	std::string GetAbsolutePathSelectedFile(std::string fileName);	//���ϸ���Ʈ�� �ִ� ������ ���� ��θ� ����
	std::string GetRelativePathSelectedFile(std::string fileName);	//���ϸ���Ʈ�� �ִ� ������ ����� ����

	//�Է��� path�� ���� �ּҸ� ����
	std::string GetAbsolutePath(std::filesystem::path fileName) {
		return fileName.is_absolute() ? fileName.string() : absolute(fileName).string();
	}

	//�Է��� path�� ��� �ּҸ� ����
	std::string GetRelativePath(std::filesystem::path fileName) {
		return fileName.is_absolute() ? relative(CurrentPath).string() : fileName.string();
	}

	//���� ����� �����ּ� ����
	std::string GetCurrentPathAbsolute() {
		return CurrentPath.is_absolute() ? CurrentPath.string() : std::filesystem::absolute(CurrentPath).string();
	}

	//���� ����� ����ּҸ� ����
	std::string GetCurrentPathRelative() {
		return CurrentPath.is_relative() ? CurrentPath.string() : std::filesystem::relative(CurrentPath).string();
	}

	std::vector<FileInfo>& GetFileList() { SearchFileList(); return FileList; }

private:
	void SearchFileList(bool saveList = true);	//���� Path�� ���ϸ�� ���. �Ű����� saveList�� true�ÿ� ����Ʈ�� ����

	//�ʵ�
private:
	std::filesystem::path RootPath;
	std::filesystem::path CurrentPath;	//���� ��� (����Ʈ ���� [���� ���α׷� ��ġ\Assets])

	std::vector<FileInfo> FileList;	//���Ϲ� ���丮 ���
};
