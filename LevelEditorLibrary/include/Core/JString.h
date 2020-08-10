#pragma once

#include "MyStringConverter.h"

#include <ostream>

//프로그램 내의 인코딩을 UTF-8로 유지하기 위한 스트링
class JString {
public:
	using iterator = std::string::iterator;
	//생성자
public:
	JString() = default;
	JString(int count, char ch = 0);
	JString(const char* str);
	JString(const std::string& str);
	~JString() = default;

	//복사 관련
	JString(const JString& rhs) = default;
	JString& operator=(const JString& rhs) = default;

	//이동 관련
	JString(JString&& rhs) noexcept;
	JString& operator=(JString&& rhs) noexcept;


	JString& operator+=(const JString& rhs);
	JString& operator+=(const char* str);

	friend std::ostream& operator <<(std::ostream& os, JString& str) {	

		if (!str.mConvStr.empty()) {
			str.mString = MyStringConverter::SystemCPToUTF8(str.mConvStr);
			str.mConvStr = TEXT("");
		}

		return os << MyStringConverter::UTF8ToSystemCP(str.mString);
	}

	//메서드
public:
	//const char* 형 반환
	const char* c_str() const;

	//comp와 문자열 비교
	int compare(const JString& comp) const;

	//비어있는지 체크
	bool empty() const;

	//문자 하나 삽입
	void push_back(const char ch);

	//문자 찾아서 pos반환. 못 찾을 시 npos 반환
	size_t find(const char ch);
	
	//시작 iterator
	std::string::iterator begin();
	//끝 iterator
	std::string::iterator end();

	//문자열 갯수 반환
	size_t size();
	size_t length();

	//char*형 버퍼를 가져온다
	char* GetCharBuffer();
	//wchar_t*형 버퍼를 가져온다
	wchar_t* GetWCharBuffer();

private:
	void Convert() const;

private:
	mutable std::string mString;	//UTF-8로 변환된 문자열을 담당할 멤버 변수
	mutable std::wstring mConvStr;	//UTF-16 버퍼를 입력받는 함수에 제공

};
