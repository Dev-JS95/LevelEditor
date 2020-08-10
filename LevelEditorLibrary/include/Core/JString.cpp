#include "JString.h"

JString::JString(int count, char ch) : 
	mString(count, ch),
	mConvStr()
{
	mString = MyStringConverter::SystemCPToUTF8(mString);
}

JString::JString(const char* str) :
	mString(str),
	mConvStr()
{
	mString = MyStringConverter::SystemCPToUTF8(mString);
}

JString::JString(const std::string& str):
	mString(str),
	mConvStr()
{
	mString = MyStringConverter::SystemCPToUTF8(mString);
}

JString::JString(JString&& rhs) noexcept
{
	mString = std::move(rhs.mString);
	mConvStr = std::move(rhs.mConvStr);
}

JString& JString::operator=(JString&& rhs) noexcept
{
	mString = std::move(rhs.mString);
	mConvStr = std::move(rhs.mConvStr);

	return *this;
}

JString& JString::operator+=(const JString& rhs)
{
	Convert();
	mString += rhs.mString;
	return *this;
}

JString& JString::operator+=(const char* str)
{
	Convert();
	mString += str;
	return *this;
}

const char* JString::c_str() const
{
	Convert();

	return mString.c_str();
}

int JString::compare(const JString& comp) const
{
	Convert();

	return mString.compare(comp.mString);
}

bool JString::empty() const
{
	Convert();

	return mString.empty();
}

void JString::push_back(const char ch)
{
	mString.push_back(ch);
}

size_t JString::find(const char ch)
{
	Convert();
	return mString.find(ch);
}

std::string::iterator JString::begin()
{
	return 	mString.begin();
}

std::string::iterator JString::end()
{
	return mString.end();
}

size_t JString::size()
{
	Convert();

	return mString.size();
}

size_t JString::length()
{
	Convert();

	return mString.length();
}

char* JString::GetCharBuffer()
{
	return &(mString[0]);
}

wchar_t* JString::GetWCharBuffer()
{
	if (mString.size() > mConvStr.size())
		mConvStr.resize(mString.size());

	return &(mConvStr[0]);
}

void JString::Convert() const
{
	if (!mConvStr.empty()) {
		mString = MyStringConverter::SystemCPToUTF8(mConvStr);
		mConvStr = TEXT("");
	}
}
