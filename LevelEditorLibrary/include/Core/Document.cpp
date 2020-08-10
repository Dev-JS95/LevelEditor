#include "Document.h"
#include <algorithm>

using namespace std;
using namespace customjson;

bool Document::ParseJsonFile(const JString& filePath) {

	std::ifstream is(filePath.c_str(), std::ios::binary);

	if (!is.is_open())
		return false;

	//파일 사이즈 체크 후 버퍼로 읽기
	is.seekg(0, std::ios::end);
	int fileSize = (int)is.tellg();
	char* buffer = new char[fileSize];
	is.seekg(0, std::ios::beg);
	is.read(buffer, fileSize);
	is.close();

	//버퍼에 있는 데이터로 파싱
	bool ret = ParseJsonFileFromMemory(buffer, fileSize);

	delete[] buffer;

	return ret;
}

bool customjson::Document::ParseJsonFileFromMemory(const char* buffer, int size)
{
	//화이트스페이스 제거(파싱을 간단히 하기 위해서)
	auto newBuffer = DeleteWhiteSpace(buffer, size);

	//파싱
	for (auto&& it = newBuffer.begin(); it != newBuffer.end(); it++) {
		//루트는 항상 오브젝트를 나타냄
		if (*it == '{') {
			if (ParseObject(it)) {
				//작업 대상 스택에 있는 값 root에 설정
				mRoot.reset(dynamic_cast<ObjectValue*>(mCurValue.top()));
				mCurValue.pop();
				break;
			}
			else
				return false;
		}
		else {
			return false;
		}
	}

	return true;
}

bool customjson::Document::SaveJsonFile(const JString& filePath)
{
	JString fileBuffer;

	//루트에 있는 오브젝트를 재귀적으로 호출하여 파일 버퍼에 데이터 입력
	if (mRoot.get() != nullptr) {
		if (mRoot.get()->IsObject()) {
			if (WriteObject(fileBuffer, mRoot.get())) {
				//버퍼 입력이 성공하면 파일로 저장(UTF8인코딩)
				ofstream of(filePath.c_str());
				of << fileBuffer;
				return true;
			}
		}
	}

	return false;
}

void customjson::Document::SetObject()
{
	//루트 오브젝트 생성
	if (mRoot.get() == nullptr)
		mRoot.reset(new ObjectValue());
}

void customjson::Document::AddMember(const JString& key, ValueType* value)
{
	//루트 오브젝트에 멤버 추가
	if (mRoot.get() != nullptr) {
		mRoot.get()->AddMember(key, value);
	}
}


JString Document::DeleteWhiteSpace(const char* buffer, int size) {
	JString noWhiteSpaceBuffer;

	bool IsString = false;
	for (int i = 0; i < size; i++) {
		char tempBuf = buffer[i];
		//문자열인지 체크
		if (!IsString && tempBuf == '"') {
			IsString = true;
		}
		else if (IsString && tempBuf == '"') {
			IsString = false;
		}

		//화이트 스페이스 제거
		else if (!IsString && (tempBuf == '\n' || tempBuf == ' ' || tempBuf == '\r' || tempBuf == '\t')) {
			continue;
		}

		noWhiteSpaceBuffer.push_back(tempBuf);
	}

	return noWhiteSpaceBuffer;
}

bool customjson::Document::ParseObject(JString::iterator& iter)
{
	//이터레이터가 문자열 끝을 나타낼 경우 false 리턴
	if (*iter == '\0')
		return false;

	//새 오브젝트를 스택에 넣어둔다.
	mCurValue.push(new ObjectValue());

	//스택 상단의 벨류를 현재 오브젝트로 설정
	auto curObj = dynamic_cast<ObjectValue*>(mCurValue.top());

	while (true) {
		iter++;

		//Key Parsing(Key는 항상 문자열)
		JString keyName = ParseString(iter);


		//Key와 Value 사이에는 콜론이 와야한다.
		if (*iter++ != ':')
			return false;


		//Value Parsing
		ValueType* value = ParseValue(iter);

		//생성한 value와 Name으로 멤버를 생성
		curObj->AddMember(keyName, value);

		//현재 이터가 ',' 면 다음 <key : value> 처리
		if (*iter == ',')
			continue;
		//'}'일 경우 오브젝트 입력 종료
		else if (*iter == '}') {
			iter++;
			break;
		}
		else {
			return false;
		}
	}

	return true;
}

bool customjson::Document::PaeseArray(JString::iterator& iter)
{
	//현재 이터레이터가 문자열 종료 코드를 가리키면 false 리턴
	if (*iter == '\0')
		return false;

	//새 배열을 스택에 넣어둔다.
	mCurValue.push(new ArrayValue());

	//작업 대상 스택 상단의 벨류를 배열벨류로 가져오기
	auto curArr = dynamic_cast<ArrayValue*>(mCurValue.top());

	while (true) {
		iter++;

		//Value Parsing
		ValueType* value = ParseValue(iter);

		//파싱한 벨류를 입력
		curArr->PushValue(value);

		//이터가 ','일 경우 다음 요소 파싱
		if (*iter == ',')
			continue;
		//']'일 경우 배열벨류 입력 종료
		else if (*iter == ']') {
			iter++;
			break;
		}
		else {
			return false;
		}
	}

	return true;
}

JString customjson::Document::ParseString(JString::iterator& iter)
{
	bool IsString = false;
	JString valueStr;
	int maxLen = 1024;
	while (maxLen-- > 0) {
		//이터가 문자열 종료 코드이면 파싱 종료
		if (*iter == '\0')
			return valueStr;

		char checkValueName = *iter++;

		if (!IsString && checkValueName == '"') {
			IsString = true;
		}
		else if (IsString && checkValueName == '"') {
			IsString = false;
			break;
		}
		else if (IsString) {
			valueStr.push_back(checkValueName);
		}
	}
	return valueStr;
}

ValueType* customjson::Document::ParseValue(JString::iterator& iter)
{
	ValueType* value = nullptr;

	if (*iter == '\0')
		return nullptr;

	//key : value 에서 콜론 다음에 
	//'"'가 존재하면 문자열로 읽기
	//'{'가 존재하면 오브젝트로 읽기
	//'['가 존재하면 배열로 읽기
	//위의 경우가 아니라면 문자 데이터들을 읽어서 데이터 분류

	//문자열
	if (*iter == '"') {
		JString valueStr = ParseString(iter);
		value = new StringValue(valueStr);
	}
	//오브젝트
	else if (*iter == '{') {
		if (!ParseObject(iter)) 
			return nullptr;

		//스택에 있는 값 현재 벨류로 설정
		value = mCurValue.top();
		mCurValue.pop();
	}
	//배열
	else if (*iter == '[') {
		if (!PaeseArray(iter))
			return nullptr;

		//스택에 있는 값 현재 벨류로 설정
		value = mCurValue.top();
		mCurValue.pop();

	}
	//한 라인을 읽고 Null 혹은 Boolean 혹은 숫자형 판단
	//읽은 데이터 중에 .이 있으면 실수형 데이터에 넣기, 아닌 경우 정수형 데이터에 저장
	else {
		int maxLen = 256;
		JString line;
		for (int i = 0; i < maxLen; i++) {
			if (('0' <= *iter && *iter <= '9') ||
				'a' <= *iter && *iter <= 'z' ||
				'A' <= *iter && *iter <= 'Z' ||
				*iter == '/' || *iter == '.' || *iter == '-' || *iter == '+')
				line.push_back(*iter);
			else
				break;
			iter++;
		}

		//읽을 수 있는 문자 데이터가 없으면 value가 없는 경우이므로 nullptr 리턴
		if (line.empty())
			return nullptr;

		//문자열 소문자로 변환
		JString lowerLine(line.length(), 0);
		std::transform(line.begin(), line.end(), lowerLine.begin(), ::tolower);

		//null인지 체크
		if (lowerLine.compare("null") == 0) {
			value = new NullValue(lowerLine);
		}
		//bool인지 체크
		else if (lowerLine.compare("true") == 0 || lowerLine.compare("false") == 0) {
			value = new NullValue(lowerLine);
		}
		//정수형
		else if (line.find('.') == string::npos) {
			int num = 0;
			sscanf_s(line.c_str(), "%d", &num);
			value = new NumericValue(num);
		}
		//실수형
		else {
			float num = 0;
			sscanf_s(line.c_str(), "%f", &num);
			value = new NumericValue(num);
		}
	}

	return value;
}


bool customjson::Document::WriteObject(JString& fileBuffer, ObjectValue* ptr)
{
	//현재 작업 대상 스택에 ptr넣기
	mCurValue.push(ptr);
	fileBuffer.push_back('{');
	fileBuffer.push_back('\n');


	auto obj = dynamic_cast<ObjectValue*>(mCurValue.top()->GetObjectValue());
	const auto& pairData = obj->GetSelf();

	for (size_t i = 0; i < pairData.size(); i++) {

		//현재 작업대상의 오브젝트 만큼 탭 혹은 공백 생성
		for (size_t i = 0; i < mCurValue.size(); i++)
			fileBuffer.push_back('\t');

		//현재 인덱스의 pairData를 받아옴
		const auto& e = pairData[i];

		//key값 넣기
		fileBuffer.push_back('"');
		fileBuffer += e.first;
		fileBuffer.push_back('"');

		//키와 벨류사이에 콜론 존재
		fileBuffer += " : ";

		//value값 넣기
		if (!WriteValue(fileBuffer, e.second.get()))
			return false;

		//마지막 데이터가 아닌 경우 쉼표 생성
		if (i != pairData.size() - 1) {
			fileBuffer.push_back(',');
		}

		fileBuffer.push_back('\n');
	}

	//작업대상에서 빼기
	mCurValue.pop();

	//현재 작업대상의 오브젝트 만큼 탭 혹은 공백 생성
	for (size_t i = 0; i < mCurValue.size(); i++)
		fileBuffer.push_back('\t');

	fileBuffer.push_back('}');

	return true;
}

bool customjson::Document::WriteArray(JString& fileBuffer, ArrayValue* ptr)
{
	//현재 작업 대상 스택에 ptr 넣기
	mCurValue.push(ptr);
	fileBuffer.push_back('[');
	fileBuffer.push_back('\n');

	auto obj = dynamic_cast<ArrayValue*>(mCurValue.top());
	const auto& arrData = obj->GetSelf();

	for (size_t i = 0; i < arrData.size(); i++) {

		//현재 작업대상의 오브젝트 만큼 탭 혹은 공백 생성
		for (size_t i = 0; i < mCurValue.size(); i++)
			fileBuffer.push_back('\t');

		//현재 인덱스의 pairData를 받아옴
		const auto& e = arrData[i];

		//value값 넣기
		if (!WriteValue(fileBuffer, e.get()))
			return false;

		//마지막 데이터가 아니면 쉼표 생성
		if (i != arrData.size() - 1) {
			fileBuffer.push_back(',');
		}

		fileBuffer.push_back('\n');
	}

	mCurValue.pop();

	//현재 작업대상의 오브젝트 만큼 탭 혹은 공백 생성
	for (size_t i = 0; i < mCurValue.size(); i++)
		fileBuffer.push_back('\t');

	fileBuffer.push_back(']');

	return true;
}

bool customjson::Document::WriteValue(JString& fileBuffer, ValueType* value)
{

	//문자열
	if (value->IsString()) {
		fileBuffer.push_back('"');
		fileBuffer += value->GetString();
		fileBuffer.push_back('"');
	}

	//오브젝트형
	else if (value->IsObject()) {
		auto tempObj = dynamic_cast<ObjectValue*>(value->GetObjectValue());
		if (!WriteObject(fileBuffer, tempObj))
			return false;
	}

	//배열형
	else if (value->IsArray()) {
		auto tempArr = dynamic_cast<ArrayValue*>(value);
		if (!WriteArray(fileBuffer, tempArr))
			return false;
	}

	//널형
	else if (value->IsNull()) {
		fileBuffer += value->GetNull();
	}

	//불린형
	else if (value->IsBoolean()) {
		if (value->GetBoolean())
			fileBuffer += "true";
		else
			fileBuffer += "false";
	}

	//숫자형
	else if (value->IsNumeric()) {
		if (value->IsInt32()) {
			int32_t num = value->GetInt32();
			char str[256] = { 0, };
			sprintf_s(str, "%d", num);
			fileBuffer += str;
		}
		else {
			float num = value->GetFloat();
			char str[256] = { 0, };
			sprintf_s(str, "%0.3f", num);
			fileBuffer += str;
		}
	}

	return true;
}
