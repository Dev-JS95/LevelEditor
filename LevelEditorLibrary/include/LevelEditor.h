#pragma once
#include <Windows.h>
#include "Renderer/Direct3D12Renderer.h"
#include "ResourceManager/ResourceManager.h"
#include "ResourceManager/AssetBrowser/AssetBrowser.h"

class LevelEditor {
	//생성자
public:
	LevelEditor(HWND handle);

	//메소드
public:
	void Tick();	//한틱마다 실행

	//에셋브라우저의 현재 경로의 파일들 리턴
	std::vector<FileInfo>& GetFileList() { return mBrowser.GetFileList(); }

private:
	void Update();
	void Draw();

	//필드
private:
	HWND hViewport;
	Direct3D12Renderer mRenderer;
	GameTimer mTimer;
	AssetBrowser mBrowser;
};