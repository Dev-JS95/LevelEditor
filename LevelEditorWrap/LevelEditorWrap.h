#pragma once

#include "LevelEditor.h"

using namespace System;

namespace LevelEditorWrap {
	/*FileType*/
	public enum class EFileType : uint8_t {
		AssetFile,
		Directory
	};

	/*FileInfo*/
	public value struct FFileInfo {
		EFileType FileType;
		System::String^ FileName;
		System::String^ AssetType;
		System::String^ Path;
	};

	public ref class LevelEditorWrapper
	{
		//생성자
	public:
		LevelEditorWrapper(IntPtr hWnd);
		virtual ~LevelEditorWrapper();

		//메서드
	public:
		void Tick();

		//에셋브라우저의 현재 경로의 파일들 리턴
		array<FFileInfo>^ GetFileList();

		//필드
	protected:
		LevelEditor* mInstance = nullptr;
	};
}
