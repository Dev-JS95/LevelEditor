#include "pch.h"

#include "LevelEditorWrap.h"
#include <msclr/marshal_cppstd.h>


namespace LevelEditorWrap {
	LevelEditorWrapper::LevelEditorWrapper(IntPtr hWnd) :
		mInstance(new LevelEditor((HWND)(HANDLE)hWnd))
	{
	}

	LevelEditorWrapper::~LevelEditorWrapper()
	{
		if (mInstance != nullptr) {
			delete mInstance;
			mInstance = nullptr;
		}
	}

	void LevelEditorWrapper::Tick()
	{
		mInstance->Tick();
	}

	array<FFileInfo>^ LevelEditorWrapper::GetFileList()
	{
		array<FFileInfo>^ ret = nullptr;

		auto& fileList = mInstance->GetFileList();

		ret = gcnew array<FFileInfo>((int)fileList.size());

		for (int i = 0; i < (int)fileList.size(); ++i) {
			FFileInfo temp;
			temp.FileName = msclr::interop::marshal_as<System::String^>(fileList[i].FileName);
			temp.FileType = (EFileType)fileList[i].FileType;
			temp.AssetType = msclr::interop::marshal_as<System::String^>(fileList[i].AssetType);
			temp.Path = msclr::interop::marshal_as<System::String^>(fileList[i].Path);
			ret[i] = temp;
		}

		return ret;
	}

}


