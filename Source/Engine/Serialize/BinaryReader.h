#pragma once
#include "../serialize/SerializeTraits.h"

namespace Unique
{
	class BinaryReader
	{
	public:

		bool IsReading() { return true; }
		bool IsWriting() { return false; }

		template<class T>
		bool Load(const String& fileName, T& data);

		template<class T>
		bool Load(BinaryReader& source, T& data);

		template<class T>
		void Transfer(T& data, const char* name, int metaFlag = 0);


		void BeginMap(int sz)
		{
		}

		void EndMap()
		{
		}

		template <typename First, typename... Rest>
		void TransferNVP(First& first, Rest&... rest)
		{
			int sz = sizeof ...(Rest)+1;
			BeginMap(sz / 2);
			TransferImpl1(first, rest...); // recursive call using pack expansion syntax  
			EndMap();
		}

		template<class T>
		void Transfer(T& data);

		template<class T>
		void TransferBasicData(T& data);

		template<class T>
		void TransferObject(SPtr<T>& data);

		template<class T>
		void TransferSTLStyleArray(T& data, int metaFlag = 0);

		template<class T>
		void TransferSTLStyleMap(T& data, int metaFlag = 0);

		template<class T>
		void TransferSTLStyleSet(T& data, int metaFlag = 0);
	protected:

		template <typename First, typename... Rest>
		void TransferImpl1(First& first, Rest&... rest)
		{
			TransferImpl2(first, rest...); // recursive call using pack expansion syntax  
		}

		template <typename First, typename Second, typename... Rest>
		void TransferImpl2(First& val, Second name, const Rest&... rest)
		{
			Transfer(val, name);
			TransferImpl1(rest...); // recursive call using pack expansion syntax  
		}

		template <typename First, typename Second, typename... Rest>
		void TransferImpl2(First& val, Second name) {
			Transfer(val, name);
		}
		int metaFlag_;
	};
	
	template<class T>
	inline	bool BinaryReader::Load(const String& fileName, T& data)
	{
		/*
		std::ifstream jsonFile(fileName.CString());
		IStreamWrapper isw(jsonFile);

		Document doc;
		if (doc.ParseStream(isw).HasParseError())
		{
			return false;
		}

		currentNode = &doc;*/
		SerializeTraits<T>::Transfer(data, *this);
		return true;
	}

	template<class T>
	inline bool BinaryReader::Load(BinaryReader& source, T& data)
	{	/*
		unsigned dataSize = source.GetSize();
		if (!dataSize && !source.GetName().Empty())
		{
			UNIQUE_LOGERROR("Zero sized JSON data in " + source.GetName());
			return false;
		}

		SharedArrayPtr<char> buffer(new char[dataSize + 1]);
		if (source.Read(buffer.Get(), dataSize) != dataSize)
			return false;
		buffer[dataSize] = '\0';

		rapidjson::Document document;
		if (document.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(buffer).HasParseError())
		{
			UNIQUE_LOGERROR("Could not parse JSON data from " + source.GetName());
			return false;
		}

		currentNode = &document;*/
		SerializeTraits<T>::Transfer(data, *this);
		return true;
	}


	template<class T>
	inline void BinaryReader::Transfer(T& data, const char* name, int metaFlag)
	{
		metaFlag_ = metaFlag;
		/*
		Value::MemberIterator node = currentNode->FindMember(name);
		if (node == currentNode->MemberEnd())
		{
			return;
		}

		Value* parentNode = currentNode;
		currentNode = &node->value;*/

		SerializeTraits<T>::Transfer(data, *this);

		//currentNode = parentNode;

	}

	template<class T>
	inline void BinaryReader::Transfer(T& data)
	{
		data.Transfer(*this);
	}

	template<class T>
	inline void BinaryReader::TransferObject(SPtr<T>& data)
	{/*
		if (data == nullptr)
		{
			Value::MemberIterator node = currentNode->FindMember("type");
			if (node == currentNode->MemberEnd())
			{
				UNIQUE_LOGWARNING("Unkown object type.");
				return;
			}

			data = StaticCast<T, Object>(Object::GetContext()->CreateObject(node->value.GetString()));
		}

		data->VirtualTransfer(*this);*/
	}

	template<class T>
	inline void BinaryReader::TransferSTLStyleArray(T& data, int metaFlag)
	{
		typedef typename NonConstContainerValueType<T>::value_type non_const_value_type;

		data.clear();
		/*
		if (!currentNode->IsArray())
		{
			assert(false);
			return;
		}

		Value* parentNode = currentNode;

		for (SizeType i = 0; i < parentNode->Size(); ++i)
		{
			auto& child = (*parentNode)[i];
			currentNode = &child;
			non_const_value_type val;

			SerializeTraits<non_const_value_type>::Transfer(val, *this);

			data.push_back(val);
		}

		currentNode = parentNode;*/

	}

	template<class T>
	inline void BinaryReader::TransferSTLStyleMap(T& data, int metaFlag)
	{
		typedef typename NonConstContainerValueType<T>::value_type non_const_value_type;
		typedef typename non_const_value_type::first_type first_type;
		typedef typename non_const_value_type::second_type second_type;

		data.clear();
		/*
		Value* parentNode = currentNode;

		for (SizeType i = 0; i < parentNode->Size(); ++i)
		{
			auto& child = (*parentNode)[i];
			currentNode = &child;
			non_const_value_type val;
			SerializeTraits<non_const_value_type>::Transfer(val, *this);
			data.insert(val);
		}

		currentNode = parentNode;*/
	}

	template<class T>
	inline void BinaryReader::TransferSTLStyleSet(T& data, int metaFlag)
	{
		typedef typename NonConstContainerValueType<T>::value_type non_const_value_type;

		data.clear();
		/*
		if (!currentNode->IsArray())
		{
			ASSERT(FALSE);
			return;
		}

		Value* parentNode = currentNode;

		for (SizeType i = 0; i < parentNode->Size(); ++i)
		{
			auto& child = parentNode->operator[][i];
			currentNode = child;
			non_const_value_type val;
			SerializeTraits<non_const_value_type>::Transfer(val, *this);
			data.insert(val);
		}

		currentNode = parentNode;*/
	}

	template<class T>
	inline void BinaryReader::TransferBasicData(T& data)
	{
	//	data = FromString<T>(currentNode->GetString());
	}

	template<>
	inline void BinaryReader::TransferBasicData<bool>(bool& data)
	{
	//	assert(currentNode->IsBool());
	//	data = currentNode->GetBool();
	}

	template<>
	inline void BinaryReader::TransferBasicData<char>(char& data)
	{
	//	assert(currentNode->IsInt());
	//	data = currentNode->GetInt();
	}

	template<>
	inline void BinaryReader::TransferBasicData<char*>(char*& data)
	{
	//	assert(currentNode->IsInt());
	//	strcpy(data, currentNode->GetString());
	}

	template<>
	inline void BinaryReader::TransferBasicData<unsigned char>(unsigned char& data)
	{
	//	assert(currentNode->IsUint());
	//	data = currentNode->GetUint();
	}

	template<>
	inline void BinaryReader::TransferBasicData<short>(short& data)
	{
	//	assert(currentNode->IsInt());
	//	data = currentNode->GetInt();
	}

	template<>
	inline void BinaryReader::TransferBasicData<unsigned short>(unsigned short& data)
	{
	//	assert(currentNode->IsUint());
	//	data = currentNode->GetUint();
	}

	template<>
	inline void BinaryReader::TransferBasicData<int>(int& data)
	{
	//	assert(currentNode->IsInt());
	//	data = currentNode->GetInt();
	}

	template<>
	inline void BinaryReader::TransferBasicData<unsigned int>(unsigned int& data)
	{
	//	assert(currentNode->IsUint());
	//	data = currentNode->GetUint();
	}

	template<>
	inline void BinaryReader::TransferBasicData<float>(float& data)
	{
	//	assert(currentNode->IsDouble());
	//	data = currentNode->GetFloat();
	}

	template<>
	inline void BinaryReader::TransferBasicData<double>(double& data)
	{
	//	assert(currentNode->IsDouble());
	//	data = currentNode->GetDouble();
	}
}
