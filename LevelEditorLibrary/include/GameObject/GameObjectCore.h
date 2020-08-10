#pragma once

using uint32_t = unsigned int;

/// <summary>
/// X65599 Hash 개량형 함수 [출처 : https://www.sysnet.pe.kr/2/0/1222]
/// </summary>
/// <param name="string">char*형 문자열</param>
/// <param name="N">문자열 크기</param>
/// <returns></returns>
uint32_t GenerateHash(char* string, int N)
{
	uint32_t hash = 0;
	uint32_t poly = 0xEDB88320;
	for (int i = 0; i < N; i++)
	{
		poly = (poly << 1) | (poly >> (32 - 1)); // 1bit Left Shift
		hash = (int)(poly * hash + string[i]);
	}

	return hash;
}

/// <summary>
/// X65599 Hash 개량형 상수 함수 [출처 : https://www.sysnet.pe.kr/2/0/1222]
/// </summary>
/// <param name="string">const char*형 문자열</param>
/// <returns></returns>
template <int N>
constexpr uint32_t GenerateHash(const char(&string)[N])
{
	uint32_t hash = 0;
	uint32_t poly = 0xEDB88320;
	for (int i = 0; i < N; i++)
	{
		poly = (poly << 1) | (poly >> (32 - 1)); // 1bit Left Shift
		hash = (int)(poly * hash + string[i]);
	}

	return hash;
}


//클래스 선언시 위에 선언
#define CLASS_REGISTER(inClass)\
public:\
virtual uint32_t GetClassId() const { return kClassId; }\
virtual const char* GetClassName() const { return name;}\
private:\
static const uint32_t kClassId = GenerateHash(#inClass); \
static GameObject* CreateInstance() { return new inClass(); }\
const char* name = #inClass;\
friend class ObjectRegister;
