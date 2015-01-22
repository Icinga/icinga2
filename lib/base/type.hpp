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

namespace icinga
{

enum FieldAttribute
{
	FAConfig = 1,
	FAState = 2,
	FAInternal = 32
}; 

class Type;

struct Field
{
	int ID;
	const char *TypeName;
	const char *Name;
	int Attributes;

	Field(int id, const char *type, const char *name, int attributes)
		: ID(id), TypeName(type), Name(name), Attributes(attributes)
	{ }
};

enum TypeAttribute
{
	TAAbstract = 1
};

class I2_BASE_API Type : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Type);

	virtual String GetName(void) const = 0;
	virtual Type::Ptr GetBaseType(void) const = 0;
	virtual int GetAttributes(void) const = 0;
	virtual int GetFieldId(const String& name) const = 0;
	virtual Field GetFieldInfo(int id) const = 0;
	virtual int GetFieldCount(void) const = 0;

	Object::Ptr Instantiate(void) const;

	bool IsAssignableFrom(const Type::Ptr& other) const;

	bool IsAbstract(void) const;

	static void Register(const Type::Ptr& type);
	static Type::Ptr GetByName(const String& name);

protected:
	virtual ObjectFactory GetFactory(void) const = 0;

private:
	typedef std::map<String, Type::Ptr> TypeMap;

	static TypeMap& GetTypes(void);
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
		INITIALIZE_ONCE(RegisterType ## type); \
	} } \
	DEFINE_TYPE_INSTANCE(type)

#define DEFINE_TYPE_INSTANCE(type) \
	Type::Ptr type::TypeInstance

}

#endif /* TYPE_H */
