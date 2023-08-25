/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/objecttype.hpp"
#include "base/initialize.hpp"
#include <boost/throw_exception.hpp>

using namespace icinga;

/* Ensure that the priority is lower than the basic namespace initialization in scriptframe.cpp. */
INITIALIZE_ONCE_WITH_PRIORITY([]() {
	Type::Ptr type = new ObjectType();
	type->SetPrototype(Object::GetPrototype());
	Type::Register(type);
	Object::TypeInstance = type;
}, InitializePriority::RegisterObjectType);

String ObjectType::GetName() const
{
	return "Object";
}

Type::Ptr ObjectType::GetBaseType() const
{
	return nullptr;
}

int ObjectType::GetAttributes() const
{
	return 0;
}

int ObjectType::GetFieldId(const String& name) const
{
	if (name == "type")
		return 0;
	else
		return -1;
}

Field ObjectType::GetFieldInfo(int id) const
{
	if (id == 0)
		return {1, "String", "type", nullptr, nullptr, 0, 0};
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

int ObjectType::GetFieldCount() const
{
	return 1;
}

ObjectFactory ObjectType::GetFactory() const
{
	return DefaultObjectFactory<Object>;
}
