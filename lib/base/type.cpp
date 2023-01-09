/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/type.hpp"
#include "base/scriptglobal.hpp"
#include "base/namespace.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

Type::Ptr Type::TypeInstance;

/* Ensure that the priority is lower than the basic namespace initialization in scriptframe.cpp. */
INITIALIZE_ONCE_WITH_PRIORITY([]() {
	Type::Ptr type = new TypeType();
	type->SetPrototype(TypeType::GetPrototype());
	Type::TypeInstance = type;
	Type::Register(type);
}, InitializePriority::RegisterTypeType);

String Type::ToString() const
{
	return "type '" + GetName() + "'";
}

void Type::Register(const Type::Ptr& type)
{
	ScriptGlobal::Set("Types." + type->GetName(), type);
}

Type::Ptr Type::GetByName(const String& name)
{
	Namespace::Ptr typesNS = ScriptGlobal::Get("Types", &Empty);

	if (!typesNS)
		return nullptr;

	Value ptype;
	if (!typesNS->Get(name, &ptype))
		return nullptr;

	if (!ptype.IsObjectType<Type>())
		return nullptr;

	return ptype;
}

std::vector<Type::Ptr> Type::GetAllTypes()
{
	std::vector<Type::Ptr> types;

	Namespace::Ptr typesNS = ScriptGlobal::Get("Types", &Empty);

	if (typesNS) {
		ObjectLock olock(typesNS);

		for (const Namespace::Pair& kv : typesNS) {
			Value value = kv.second->Get();

			if (value.IsObjectType<Type>())
				types.push_back(value);
		}
	}

	return types;
}

String Type::GetPluralName() const
{
	String name = GetName();

	if (name.GetLength() >= 2 && name[name.GetLength() - 1] == 'y' &&
		name.SubStr(name.GetLength() - 2, 1).FindFirstOf("aeiou") == String::NPos)
		return name.SubStr(0, name.GetLength() - 1) + "ies";
	else
		return name + "s";
}

Object::Ptr Type::Instantiate(const std::vector<Value>& args) const
{
	ObjectFactory factory = GetFactory();

	if (!factory)
		BOOST_THROW_EXCEPTION(std::runtime_error("Type does not have a factory function."));

	return factory(args);
}

bool Type::IsAbstract() const
{
	return ((GetAttributes() & TAAbstract) != 0);
}

bool Type::IsAssignableFrom(const Type::Ptr& other) const
{
	for (Type::Ptr t = other; t; t = t->GetBaseType()) {
		if (t.get() == this)
			return true;
	}

	return false;
}

Object::Ptr Type::GetPrototype() const
{
	return m_Prototype;
}

void Type::SetPrototype(const Object::Ptr& object)
{
	m_Prototype = object;
}

void Type::SetField(int id, const Value& value, bool suppress_events, const Value& cookie)
{
	if (id == 1) {
		SetPrototype(value);
		return;
	}

	Object::SetField(id, value, suppress_events, cookie);
}

Value Type::GetField(int id) const
{
	int real_id = id - Object::TypeInstance->GetFieldCount();
	if (real_id < 0)
		return Object::GetField(id);

	if (real_id == 0)
		return GetName();
	else if (real_id == 1)
		return GetPrototype();
	else if (real_id == 2)
		return GetBaseType();

	BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

const std::unordered_set<Type*>& Type::GetLoadDependencies() const
{
	static const std::unordered_set<Type*> noDeps;
	return noDeps;
}

int Type::GetActivationPriority() const
{
	return 0;
}

void Type::RegisterAttributeHandler(int fieldId, const AttributeHandler& callback)
{
	throw std::runtime_error("Invalid field ID.");
}

String TypeType::GetName() const
{
	return "Type";
}

Type::Ptr TypeType::GetBaseType() const
{
	return Object::TypeInstance;
}

int TypeType::GetAttributes() const
{
	return 0;
}

int TypeType::GetFieldId(const String& name) const
{
	int base_field_count = GetBaseType()->GetFieldCount();

	if (name == "name")
		return base_field_count + 0;
	else if (name == "prototype")
		return base_field_count + 1;
	else if (name == "base")
		return base_field_count + 2;

	return GetBaseType()->GetFieldId(name);
}

Field TypeType::GetFieldInfo(int id) const
{
	int real_id = id - GetBaseType()->GetFieldCount();
	if (real_id < 0)
		return GetBaseType()->GetFieldInfo(id);

	if (real_id == 0)
		return {0, "String", "name", "", nullptr, 0, 0};
	else if (real_id == 1)
		return Field(1, "Object", "prototype", "", nullptr, 0, 0);
	else if (real_id == 2)
		return Field(2, "Type", "base", "", nullptr, 0, 0);

	throw std::runtime_error("Invalid field ID.");
}

int TypeType::GetFieldCount() const
{
	return GetBaseType()->GetFieldCount() + 3;
}

ObjectFactory TypeType::GetFactory() const
{
	return nullptr;
}

