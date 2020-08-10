#include "MyStringConverter.h"


std::wstring MyStringConverter::UTF8ToUTF16(const std::string& utf8Str)
{
	//유니코드로 변환하기 전 길이를 체크
	int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.length(), NULL, NULL);

	std::wstring convStr(nLen, 0);

	// 이제 변환을 수행한다.
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), (int)utf8Str.length(), &convStr[0], nLen);

	return convStr;
}

std::string MyStringConverter::SystemCPToUTF8(const std::string& str)
{
	//현재 시스템의 코드페이지를 체크
	UINT curSystemCP = GetACP();
	
	//시스템 코드 페이지가 UTF8이 아닌 경우
	if (curSystemCP != CP_UTF8) {

		//시스템 코드페이지에 따른 길이 체크
		int nLen = MultiByteToWideChar(curSystemCP, 0, str.c_str(), (int)str.length(), NULL, NULL);

		std::wstring convUTF16Str(nLen, 0);
		
		// 변환을 수행
		MultiByteToWideChar(curSystemCP, 0, str.c_str(), (int)str.length(), &convUTF16Str[0], nLen);

		// 다시 와이드문자열에서 utf-8문자열로 변환
		int len = WideCharToMultiByte(CP_UTF8, 0, convUTF16Str.c_str(), -1, NULL, 0, NULL, NULL);
		std::string strUTF8(len, 0);
		WideCharToMultiByte(CP_UTF8, 0, convUTF16Str.c_str(), -1, &strUTF8[0], len - 1, NULL, NULL);

		//끝에 문자열 종료 코드가 추가되는 경우 제거
		if (strUTF8.back() == '\0') 
			strUTF8 = strUTF8.substr(0, strUTF8.length() - 1);
		
		return strUTF8;
	}
	else {
		return str;
	}

	return std::string();
}

std::string MyStringConverter::SystemCPToUTF8(const std::wstring& str)
{
	//길이체크
	int len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
	std::string strUTF8(len, 0);

	//와이드 문자열에서 UTF8문자열로 변환
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &strUTF8[0], len - 1, NULL, NULL);

	if (strUTF8.back() == '\0')
		strUTF8 = strUTF8.substr(0, strUTF8.length() - 1);

	return strUTF8;
}

std::string MyStringConverter::UTF8ToSystemCP(const std::string& str)
{
	//현재 시스템 코드페이지 체크
	UINT curSystemCP = GetACP();

	//시스템 코드페이지가 UTF8이 아닌경우 와이드문자 -> 시스템 코드페이지로 바꾸어야 함
	if (curSystemCP != CP_UTF8) {

		int nLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, NULL);

		std::wstring convUTF16Str(nLen, 0);

		// 이제 변환을 수행한다.
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &convUTF16Str[0], nLen);

		// 시스템 코드페이지 문자열로 변환
		int len = WideCharToMultiByte(curSystemCP, 0, convUTF16Str.c_str(), -1, NULL, 0, NULL, NULL);
		std::string strSystemCP(len, 0);
		WideCharToMultiByte(curSystemCP, 0, convUTF16Str.c_str(), -1, &strSystemCP[0], len - 1, NULL, NULL);

		if (strSystemCP.back() == '\0')
			strSystemCP = strSystemCP.substr(0, strSystemCP.length() - 1);

		return strSystemCP;
	}
	//시스템 코드페이지가 UTF8인경우 프로그램 내부 CP와 같으므로 그대로 내보내기
	else {
		return str;
	}

	return std::string();
}
