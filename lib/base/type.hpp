/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/object.hpp"
#include "base/initialize.hpp"
#include <unordered_set>
#include <vector>

namespace icinga
{

/* keep this in sync with tools/mkclass/classcompiler.hpp */
enum FieldAttribute
{
	FAEphemeral = 1,
	FAConfig = 2,
	FAState = 4,
	FARequired = 256,
	FANavigation = 512,
	FANoUserModify = 1024,
	FANoUserView = 2048,
	FADeprecated = 4096,
};

class Type;

struct Field
{
	int ID;
	const char *TypeName;
	const char *Name;
	const char *NavigationName;
	const char *RefTypeName;
	int Attributes;
	int ArrayRank;

	Field(int id, const char *type, const char *name, const char *navigationName, const char *reftype, int attributes, int arrayRank)
		: ID(id), TypeName(type), Name(name), NavigationName(navigationName), RefTypeName(reftype), Attributes(attributes), ArrayRank(arrayRank)
	{ }
};

enum TypeAttribute
{
	TAAbstract = 1
};

class ValidationUtils
{
public:
	virtual bool ValidateName(const String& type, const String& name) const = 0;
};

class Type : public Object
{
public:
	DECLARE_OBJECT(Type);

	String ToString() const override;

	virtual String GetName() const = 0;
	virtual Type::Ptr GetBaseType() const = 0;
	virtual int GetAttributes() const = 0;
	virtual int GetFieldId(const String& name) const = 0;
	virtual Field GetFieldInfo(int id) const = 0;
	virtual int GetFieldCount() const = 0;

	String GetPluralName() const;

	Object::Ptr Instantiate(const std::vector<Value>& args) const;

	bool IsAssignableFrom(const Type::Ptr& other) const;

	bool IsAbstract() const;

	Object::Ptr GetPrototype() const;
	void SetPrototype(const Object::Ptr& object);

	static void Register(const Type::Ptr& type);
	static Type::Ptr GetByName(const String& name);
	static std::vector<Type::Ptr> GetAllTypes();

	void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty) override;
	Value GetField(int id) const override;

	virtual const std::unordered_set<Type*>& GetLoadDependencies() const;
	virtual int GetActivationPriority() const;

	typedef std::function<void (const Object::Ptr&, const Value&)> AttributeHandler;
	virtual void RegisterAttributeHandler(int fieldId, const AttributeHandler& callback);

protected:
	virtual ObjectFactory GetFactory() const = 0;

private:
	Object::Ptr m_Prototype;
};

class TypeType final : public Type
{
public:
	DECLARE_PTR_TYPEDEFS(Type);

	String GetName() const override;
	Type::Ptr GetBaseType() const override;
	int GetAttributes() const override;
	int GetFieldId(const String& name) const override;
	Field GetFieldInfo(int id) const override;
	int GetFieldCount() const override;

	static Object::Ptr GetPrototype();

protected:
	ObjectFactory GetFactory() const override;
};

template<typename T>
class TypeImpl
{
};

/* Ensure that the priority is lower than the basic namespace initialization in scriptframe.cpp. */
#define REGISTER_TYPE(type) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		icinga::Type::Ptr t = new TypeImpl<type>(); \
		type::TypeInstance = t; \
		icinga::Type::Register(t); \
	}, InitializePriority::RegisterTypes); \
	DEFINE_TYPE_INSTANCE(type)

#define REGISTER_TYPE_WITH_PROTOTYPE(type, prototype) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		icinga::Type::Ptr t = new TypeImpl<type>(); \
		t->SetPrototype(prototype); \
		type::TypeInstance = t; \
		icinga::Type::Register(t); \
	}, InitializePriority::RegisterTypes); \
	DEFINE_TYPE_INSTANCE(type)

#define DEFINE_TYPE_INSTANCE(type) \
	Type::Ptr type::TypeInstance

}
