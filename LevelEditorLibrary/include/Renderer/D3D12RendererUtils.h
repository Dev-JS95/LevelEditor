#pragma once
#include "DDSTextureLoader.h"
#include "WICTextureLoader12.h"

#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <comdef.h>
#include <codecvt>
#include <stdexcept>
#include <memory>


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    if(FAILED(hr__)) { throw D3DException(hr__, L#x); }                \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif


std::wstring AnsiToWString(const std::string& str);	//Ansi���� WString���� ��ȯ
std::wstring StringToWString(const std::string& str);	//String���� WString���� ��ȯ
std::string WStringToString(const std::wstring& wstr);	//WString���� String���� ��ȯ


class D3DException {

    //������
public:
    D3DException() = default;
    D3DException(HRESULT hr, const std::wstring& functionName) 
        :mErrorCode(hr), 
        mFunctionName(functionName){}

    //�޼���
public:
    std::wstring ToString()const;	//���� ���ڿ��� ��ȯ

    //���
private:
    HRESULT mErrorCode;
    std::wstring mFunctionName;
};

class D3DUtil
{
public:
	//Ű ���ȴ��� üũ
	static bool IsKeyDown(int vkeyCode);

	//HRESULT�� ���ڿ��� ��ȯ
	static std::string ToString(HRESULT hr);

	//������� ����Ʈ ������ ���(�ϵ���� �Ҵ�ũ���� ������� �ϱ� ������)
	static UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		//��� ���۴� �ϵ���� �Ҵ� ũ���� ������� �Ѵ�(���� 256����Ʈ)
		// Example: Suppose byteSize = 300.
		// (300 + 255) & ~255
		// 555 & ~255
		// 0x022B & ~0x00ff
		// 0x022B & 0xff00
		// 0x0200
		// 512
		return (byteSize + 255) & ~255;
	}

	//���̴� ���̳ʸ� ���� �ε�
	static ATL::CComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	//����Ʈ ���� ����
	static ATL::CComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		ATL::CComPtr<ID3D12Resource>& uploadBuffer);

	//���̴� ������
	static ATL::CComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

};