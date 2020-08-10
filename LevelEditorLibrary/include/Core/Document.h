#pragma once
#include <fstream>
#include <stack>
#include "ValueType.h"

namespace customjson {

	class Document {
	public:
		ValueType* operator[](const JString& key) { return (*mRoot.get())[key]; }

	public:
		//JSON파일을 파싱한다.
		bool ParseJsonFile(const JString& filePath);
		//버퍼에 있는 JSON 데이터로부터 파싱
		bool ParseJsonFileFromMemory(const char* buffer, int size);


		//JSON파일로 저장 만든다
		bool SaveJsonFile(const JString& filePath);

		//루트 오브젝트의 멤버 접근
		bool HasMember(const JString& key) { return (*mRoot.get()).HasMember(key); }

		//현재 루트 도큐먼트의 값 가져오기
		ObjectValue* GetSelf() { return mRoot.get(); }

		//루트 도큐먼트 생성
		void SetObject();

		//멤버 추가
		void AddMember(const JString& key, ValueType* value);

	private:
		//Json을 파싱하기 위해 화이트 스페이스 제거
		JString DeleteWhiteSpace(const char* buffer, int size);

		//오브젝트형 value를 파싱하기 위한 메서드(재귀적으로 호출)
		bool ParseObject(JString::iterator& iter);

		//배열형 value를 파싱하기 위한 메서드(재귀적으로 호출)
		bool PaeseArray(JString::iterator& iter);

		//문자열형 value를 파싱하기 위한 메서드
		JString ParseString(JString::iterator& iter);

		//Value에 오는 값 파싱 후 생성
		ValueType* ParseValue(JString::iterator& iter);


		//버퍼에 오브젝트 데이터 쓰기
		bool WriteObject(JString& fileBuffer, ObjectValue* ptr);

		//버퍼에 배열 데이터 쓰기
		bool WriteArray(JString& fileBuffer, ArrayValue* ptr);

		//버퍼에 밸류 데이터 쓰기
		bool WriteValue(JString& fileBuffer, ValueType* value);


	private:
		std::stack<ValueType*> mCurValue;	//현재 작업대상 벨류를 담을 스택
		std::unique_ptr<ObjectValue> mRoot;	//루트 오브젝트
	};
}
