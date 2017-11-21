/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef TYPE_H
#define TYPE_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/object.hpp"
#include "base/initialize.hpp"
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

class I2_BASE_API Type : public Object
{
public:
	DECLARE_OBJECT(Type);

	virtual String ToString(void) const override;

	virtual String GetName(void) const = 0;
	virtual Type::Ptr GetBaseType(void) const = 0;
	virtual int GetAttributes(void) const = 0;
	virtual int GetFieldId(const String& name) const = 0;
	virtual Field GetFieldInfo(int id) const = 0;
	virtual int GetFieldCount(void) const = 0;

	String GetPluralName(void) const;

	Object::Ptr Instantiate(const std::vector<Value>& args) const;

	bool IsAssignableFrom(const Type::Ptr& other) const;

	bool IsAbstract(void) const;

	Object::Ptr GetPrototype(void) const;
	void SetPrototype(const Object::Ptr& object);

	static void Register(const Type::Ptr& type);
	static Type::Ptr GetByName(const String& name);
	static std::vector<Type::Ptr> GetAllTypes(void);

	virtual void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty) override;
	virtual Value GetField(int id) const override;

	virtual std::vector<String> GetLoadDependencies(void) const;
	
	typedef std::function<void (const Object::Ptr&, const Value&)> AttributeHandler;
	virtual void RegisterAttributeHandler(int fieldId, const AttributeHandler& callback);

protected:
	virtual ObjectFactory GetFactory(void) const = 0;

private:
	Object::Ptr m_Prototype;
};

class I2_BASE_API TypeType : public Type
{
public:
	DECLARE_PTR_TYPEDEFS(Type);

	virtual String GetName(void) const override;
	virtual Type::Ptr GetBaseType(void) const override;
	virtual int GetAttributes(void) const override;
	virtual int GetFieldId(const String& name) const override;
	virtual Field GetFieldInfo(int id) const override;
	virtual int GetFieldCount(void) const override;
	
	static Object::Ptr GetPrototype(void);

protected:
	virtual ObjectFactory GetFactory(void) const override;
};

template<typename T>
class I2_BASE_API TypeImpl
{
};

#define REGISTER_TYPE(type) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		icinga::Type::Ptr t = new TypeImpl<type>(); \
		type::TypeInstance = t; \
		icinga::Type::Register(t); \
	}, 10); \
	DEFINE_TYPE_INSTANCE(type)

#define REGISTER_TYPE_WITH_PROTOTYPE(type, prototype) \
	INITIALIZE_ONCE_WITH_PRIORITY([]() { \
		icinga::Type::Ptr t = new TypeImpl<type>(); \
		t->SetPrototype(prototype); \
		type::TypeInstance = t; \
		icinga::Type::Register(t); \
	}, 10); \
	DEFINE_TYPE_INSTANCE(type)

#define DEFINE_TYPE_INSTANCE(type) \
	Type::Ptr type::TypeInstance

}

#endif /* TYPE_H */
