#pragma once
#include "../Container/StringID.h"
#include "../Container/Container.h"
#include "AttributeTraits.h"

namespace Unique
{
	class Attribute;

	/// Type info.
	class UNIQUE_API TypeInfo
	{
	public:
		/// Construct.
		TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo);
		/// Destruct.
		~TypeInfo();

		/// Check current type is type of specified type.
		bool IsTypeOf(const StringID& type) const;
		/// Check current type is type of specified type.
		bool IsTypeOf(const TypeInfo* typeInfo) const;
		/// Check current type is type of specified class type.
		template<typename T> bool IsTypeOf() const { return IsTypeOf(T::GetTypeInfoStatic()); }

		/// Return type.
		const StringID& GetType() const { return type_; }
		/// Return base type info.
		const TypeInfo* GetBaseTypeInfo() const { return baseTypeInfo_; }
		
		/*
		template<class C, class T>
		void RegisterAttribute(const char* name, T C::* m, AttributeFlag flag = AttributeFlag::Default)
		{
			RegisterAttribute(
				new Unique::TAttribute<T>(name, OffsetOf(m), flag));
		}*/

		template<class T>
		void RegisterAttribute(const char* name, size_t offset, AttributeFlag flag = AttributeFlag::Default)
		{
			RegisterAttribute(
				new Unique::TAttribute<T>(name, offset, flag));
		}

		template<typename GET, typename SET>
		void RegisterAccessor(const char* name, GET getter, SET setter, AttributeFlag flag = AttributeFlag::Default)
		{
			RegisterAttribute(
				new Unique::AttributeAccessorImpl<function_traits<GET>::ClassType, function_traits<GET>::RawType, function_traits<GET>>(name, getter, setter, flag));
		}

		template<typename GET, typename SET>
		void RegisterMixedAccessor(const char* name, GET getter, SET setter, AttributeFlag flag = AttributeFlag::Default)
		{
			RegisterAttribute(
				new Unique::AttributeAccessorImpl<mixed_function_traits<GET>::ClassType, mixed_function_traits<GET>::RawType, mixed_function_traits<GET>>(name, getter, setter, flag));
		}

		void RegisterAttribute(Attribute* attr);

		const Vector<SPtr<Attribute>>& GetAttributes() const { return attributes_;}

		template<class T>
		static const Unique::StringID& TypeName()
		{
			static const Unique::StringID eventID(GetTypeName(typeid(T).name()));
			return eventID;
		}
		
	private:

		static const char* GetTypeName(const char* name)
		{
			const char* p = std::strrchr(name, ':');
			return p ? ++p : name;
		}

		/// Type.
		StringID type_;
		// Category
		StringID category_;
		/// Base class type info.
		const TypeInfo* baseTypeInfo_;

		Vector<SPtr<Attribute>> attributes_;
	};
	
	/// Define an attribute that points to a memory offset in the object.
//#define uAttribute(name, variable, ...)\
//	ClassName::GetTypeInfoStatic()->RegisterAttribute(name, &ClassName::variable, ##__VA_ARGS__);
	
#define uAttribute(name, variable, ...)\
	ClassName::GetTypeInfoStatic()->RegisterAttribute<decltype(((ClassName*)0)->variable)>(name, offsetof(ClassName, variable), ##__VA_ARGS__);

#define uAccessor(name, getFunction, setFunction, ...)\
	ClassName::GetTypeInfoStatic()->RegisterAccessor(name, &ClassName::getFunction, &ClassName::setFunction, ##__VA_ARGS__);

#define uMixedAccessor(name, getFunction, setFunction, ...)\
	ClassName::GetTypeInfoStatic()->RegisterMixedAccessor(name, &ClassName::getFunction, &ClassName::setFunction, ##__VA_ARGS__);

}