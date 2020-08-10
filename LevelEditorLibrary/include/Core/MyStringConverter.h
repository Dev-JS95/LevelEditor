#pragma once
#include <string>
#include <comutil.h>

//CP949 시스템과 CP65001시스템을 지원하기 위한 유틸리티 클래스
class MyStringConverter {

	//메서드
public:
	//utf-8문자열을 utf-16문자열로 교체
	static std::wstring UTF8ToUTF16(const std::string& utf8Str);

	//시스템의 코드페이지에 맞춰서 문자열을 UTF-8로 변경한다
	static std::string SystemCPToUTF8(const std::string& str);

	//시스템의 코드페이지에 맞춰서 wchar_t형을 UTF-8 string형으로 변경한다
	static std::string SystemCPToUTF8(const std::wstring& str);

	//프로그램 내부에선 UTF8을 사용하므로 내부 문자열을 외부로 출력할 때 코드페이지에 맞게 변환
	static std::string UTF8ToSystemCP(const std::string& str);
};
