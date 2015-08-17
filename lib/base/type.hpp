/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include <boost/function.hpp>
#include <vector>

namespace icinga
{

/* keep this in sync with tools/mkclass/classcompiler.hpp */
enum FieldAttribute
{
	FAEphemeral = 1,
	FAConfig = 2,
	FAState = 4,
	FAInternal = 64,
	FARequired = 512
}; 

class Type;

struct Field
{
	int ID;
	const char *TypeName;
	const char *Name;
	const char *RefTypeName;
	int Attributes;

	Field(int id, const char *type, const char *name, const char *reftype, int attributes)
		: ID(id), TypeName(type), Name(name), RefTypeName(reftype), Attributes(attributes)
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

	virtual String ToString(void) const;

	virtual String GetName(void) const = 0;
	virtual Type::Ptr GetBaseType(void) const = 0;
	virtual int GetAttributes(void) const = 0;
	virtual int GetFieldId(const String& name) const = 0;
	virtual Field GetFieldInfo(int id) const = 0;
	virtual int GetFieldCount(void) const = 0;

	String GetPluralName(void) const;

	Object::Ptr Instantiate(void) const;

	bool IsAssignableFrom(const Type::Ptr& other) const;

	bool IsAbstract(void) const;

	Object::Ptr GetPrototype(void) const;
	void SetPrototype(const Object::Ptr& object);

	static void Register(const Type::Ptr& type);
	static Type::Ptr GetByName(const String& name);

	virtual void SetField(int id, const Value& value, bool suppress_events = false, const Value& cookie = Empty);
	virtual Value GetField(int id) const;

	virtual std::vector<String> GetLoadDependencies(void) const;
	
	typedef boost::function<void (const Object::Ptr&, const Value&)> AttributeHandler;
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

	virtual String GetName(void) const;
	virtual Type::Ptr GetBaseType(void) const;
	virtual int GetAttributes(void) const;
	virtual int GetFieldId(const String& name) const;
	virtual Field GetFieldInfo(int id) const;
	virtual int GetFieldCount(void) const;
	
	static Object::Ptr GetPrototype(void);

protected:
	virtual ObjectFactory GetFactory(void) const;
};

template<typename T>
class TypeImpl
{
};

#define REGISTER_TYPE(type) \
	namespace { namespace UNIQUE_NAME(rt) { \
		void RegisterType ## type(void) \
		{ \
			icinga::Type::Ptr t = new TypeImpl<type>(); \
			type::TypeInstance = t; \
			icinga::Type::Register(t); \
		} \
		\
		INITIALIZE_ONCE_WITH_PRIORITY(RegisterType ## type, 10); \
	} } \
	DEFINE_TYPE_INSTANCE(type)

#define REGISTER_TYPE_WITH_PROTOTYPE(type, prototype) \
	namespace { namespace UNIQUE_NAME(rt) { \
		void RegisterType ## type(void) \
		{ \
			icinga::Type::Ptr t = new TypeImpl<type>(); \
			t->SetPrototype(prototype); \
			type::TypeInstance = t; \
			icinga::Type::Register(t); \
		} \
		\
		INITIALIZE_ONCE_WITH_PRIORITY(RegisterType ## type, 10); \
	} } \
	DEFINE_TYPE_INSTANCE(type)

#define DEFINE_TYPE_INSTANCE(type) \
	Type::Ptr type::TypeInstance

}

#endif /* TYPE_H */
