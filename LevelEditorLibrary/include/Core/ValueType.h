#pragma once
#include <memory>
#include <vector>
#include "JString.h"

namespace customjson {
	enum { kValue, kObject, kArray, kString, kNumeric, kBoolean, kNull};

	//Value의 조상 클래스
	class ValueType {

	public:
		ValueType() = default;
		virtual ~ValueType() = default;


	public:
		virtual ValueType* GetObjectValue() { return nullptr; }
		virtual ValueType* GetArray() { return nullptr; }
		virtual JString GetString() { return JString(); };
		virtual int32_t GetInt32() { return INT32_MIN; }
		virtual float GetFloat() { return 0; }
		virtual bool GetBoolean() { return false; }
		virtual JString GetNull() { return JString(); }

		virtual bool IsString() const { return false; }
		virtual bool IsObject() const { return false; }
		virtual bool IsArray() const { return false; }
		virtual bool IsNumeric() const { return false; }
		virtual bool IsInt32() const { return false; }
		virtual bool IsFloat() const { return false; }
		virtual bool IsBoolean() const { return false; }
		virtual bool IsNull() const { return false; }

		virtual ValueType* operator[](const JString& key) { return nullptr; }

	private:
		const int typeId = kValue;
	};


	//오브젝트형
	class ObjectValue : public ValueType {

		//생성자
	public:
		ObjectValue() = default;
		~ObjectValue() = default;
		ObjectValue(const ObjectValue& rhs) = delete;
		ObjectValue& operator=(const ObjectValue& rhs) = delete;

		ObjectValue(ObjectValue&& rhs) noexcept {
			for (auto& e : rhs.mData)  {
				mData.push_back(std::pair<JString, std::unique_ptr<ValueType>>(e.first, move(e.second)));
			}
		}

		ObjectValue& operator=(ObjectValue&& rhs) noexcept {
			for (auto& e : rhs.mData) {
				mData.push_back(std::pair<JString, std::unique_ptr<ValueType>>(e.first, move(e.second)));
			}
			return *this;
		}

		ValueType* operator[](const JString& key) {
			for (const auto& e : mData) {
				if (e.first.compare(key) == 0)
					return e.second.get();
			}
			return nullptr;
		}

		//메서드
	public:
		//멤버 조회
		bool HasMember(const JString& key) {
			for (const auto& e : mData) 
				if (e.first.compare(key) == 0)
					return true;
			
			return false;
		}

		//멤버 추가
		void AddMember(const JString& key, ValueType* value) {
			mData.push_back(std::pair<JString, std::unique_ptr<ValueType>>(key, std::move(value)));
		}

		//자신의 데이터 가져오기
		std::vector<std::pair<JString, std::unique_ptr<ValueType>>>& GetSelf() { return mData; }

		//오브젝트 형으로 값 가져오기
		virtual ObjectValue* GetObjectValue() override {
			return dynamic_cast<ObjectValue*>(this);
		};

		//오브젝트인 경우 true 리턴
		virtual bool IsObject() const override { return true; }


		//필드
	private:
		std::vector<std::pair<JString, std::unique_ptr<ValueType>>> mData;
		const int typeId = kObject;
	};


	//배열형
	class ArrayValue : public ValueType {

		//생성자
	public:
		ArrayValue() = default;
		~ArrayValue() = default;
		ArrayValue(const ArrayValue& rhs) = delete;
		ArrayValue& operator=(const ArrayValue& rhs) = delete;

		ArrayValue(ArrayValue&& rhs) noexcept {
			for (auto& e : rhs.mData) {
				mData.push_back(std::unique_ptr<ValueType>(move(e)));
			}
		}

		ArrayValue& operator=(ArrayValue&& rhs) noexcept {
			for (auto& e : rhs.mData) {
				mData.push_back(std::unique_ptr<ValueType>(move(e)));
			}
			return *this;
		}

		ValueType* operator[](int idx) { return mData[idx].get(); }

		//메서드
	public:
		//배열에 데이터 넣기
		void PushValue(ValueType* value) {
			auto e = std::unique_ptr<ValueType>(value);
			if(e.get() != nullptr)
				mData.push_back(std::move(e));
		}

		//배열 데이터 가져오기
		std::vector<std::unique_ptr<ValueType>>& GetSelf() { return mData; }

		//배열형으로 가져오기
		ArrayValue* GetArray() override {
			return dynamic_cast<ArrayValue*>(this);
		}

		//배열인 경우 true 리턴
		virtual bool IsArray() const override{ return true; }


		//필드
	private:
		std::vector<std::unique_ptr<ValueType>> mData;
		const int typeId = kArray;

	};


	//문자열형
	class StringValue : public ValueType {

		//생성자
	public:
		StringValue(const JString& str) : mData(str) {}

		//메서드
	public:
		//스트링 가져오기
		JString GetString() override {
			return mData;
		}

		//스트링인 경우 true리턴
		virtual bool IsString() const override { return true; }

		//필드
	private:
		JString mData;
		const int typeId = kString;
	};


	//숫자형
	class NumericValue : public ValueType {

		//32비트 int(정수)혹은 float(실수)을 담을 공용체용 구조체
		struct Number32 {
			union {
				int32_t Int = INT32_MIN;
				float Float;
			};
			bool IsInt = true;
		};

		//생성자
	public:
		explicit NumericValue(int num) { mData.Int = num; }
		explicit NumericValue(float num) { mData.Float = num; mData.IsInt = false; }

		//메서드
	public:
		//정수형으로 데이터 받기
		int32_t GetInt32() override {
			return mData.Int;
		}
		//실수형으로 데이터 받기
		float GetFloat() { return mData.Float; }


		virtual bool IsNumeric() const override { return true; }
		virtual bool IsInt32() const override { return mData.IsInt; }
		virtual bool IsFloat() const override { return !mData.IsInt; }

		//필드
	private:
		Number32 mData;
		const int typeId = kNumeric;

	};

	//불린형 데이터
	class BoolValue : public ValueType {

		//생성자
	public:
		BoolValue(bool value) {
			if (value)
				mData = "true";
			else
				mData = "false";
		}

		//메서드
	public:
		//불린 값 가져오기
		virtual bool GetBoolean() override {
			if (mData.compare("true") == 0)
				return true;
			else
				return false;
		}

		//불린데이터일 경우 true 리턴
		virtual bool IsBoolean() const override { return true; }

		//필드
	private:
		JString mData;
		const int typeId = kBoolean;
	};


	//널형 데이터
	class NullValue : public ValueType {

		//생성자
	public:
		NullValue() : mData("null") {}
		NullValue(const JString& value) : mData(value) {}

		//메서드
	public:
		//널 스트링 가져오기
		virtual JString GetNull() override {
			return mData;
		}

		//Null데이터일 경우 true 리턴
		virtual bool IsNull() const override { return true; }

		//필드
	private:
		JString mData;
		const int typeId = kNull;
	};

}

