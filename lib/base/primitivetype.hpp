/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/type.hpp"
#include "base/initialize.hpp"

namespace icinga
{

class PrimitiveType final : public Type
{
public:
	PrimitiveType(String name, String base, const ObjectFactory& factory = ObjectFactory());

	String GetName() const override;
	Type::Ptr GetBaseType() const override;
	int GetAttributes() const override;
	int GetFieldId(const String& name) const override;
	Field GetFieldInfo(int id) const override;
	int GetFieldCount() const override;

protected:
	ObjectFactory GetFactory() const override;

private:
	String m_Name;
	String m_Base;
	ObjectFactory m_Factory;
};

/* Ensure that the priority is lower than the basic namespace initialization in scriptframe.cpp. */
#define REGISTER_BUILTIN_TYPE(type, prototype)					\
	INITIALIZE_ONCE_WITH_PRIORITY([]() {					\
		icinga::Type::Ptr t = new PrimitiveType(#type, "None"); 	\
		t->SetPrototype(prototype);					\
		icinga::Type::Register(t);					\
	}, InitializePriority::RegisterBuiltinTypes)

#define REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, factory)		\
	INITIALIZE_ONCE_WITH_PRIORITY([]() {					\
		icinga::Type::Ptr t = new PrimitiveType(#type, #base, factory);	\
		t->SetPrototype(prototype);					\
		icinga::Type::Register(t);					\
		type::TypeInstance = t;						\
	}, InitializePriority::RegisterPrimitiveTypes);	\
	DEFINE_TYPE_INSTANCE(type)

#define REGISTER_PRIMITIVE_TYPE(type, base, prototype)				\
	REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, DefaultObjectFactory<type>)

#define REGISTER_PRIMITIVE_TYPE_VA(type, base, prototype)				\
	REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, DefaultObjectFactoryVA<type>)

#define REGISTER_PRIMITIVE_TYPE_NOINST(type, base, prototype)			\
	REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, nullptr)

}
