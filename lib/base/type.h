/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/i2-base.h"
#include "base/qstring.h"
#include "base/serializer.h"
#include "base/initialize.h"
#include <boost/function.hpp>

namespace icinga
{

struct Field
{
	int ID;
	const char *Name;
	int Attributes;

	Field(int id, const char *name, int attributes)
		: ID(id), Name(name), Attributes(attributes)
	{ }
};

enum TypeAttribute
{
	TAAbstract = 1,
	TASafe = 2
};

class I2_BASE_API Type
{
public:
	typedef boost::function<Object::Ptr (void)> Factory;

	virtual String GetName(void) const = 0;
	virtual const Type *GetBaseType(void) const = 0;
	virtual int GetAttributes(void) const = 0;
	virtual int GetFieldId(const String& name) const = 0;
	virtual Field GetFieldInfo(int id) const = 0;
	virtual int GetFieldCount(void) const = 0;

	Object::Ptr Instantiate(void) const;

	bool IsAssignableFrom(const Type *other) const;

	bool IsAbstract(void) const;
	bool IsSafe(void) const;

	static void Register(const Type *type);
	static const Type *GetByName(const String& name);

	void SetFactory(const Factory& factory);

private:
	typedef std::map<String, const Type *> TypeMap;

	static TypeMap& GetTypes(void);

	Factory m_Factory;
};

template<typename T>
class TypeImpl
{
};

template<typename T>
shared_ptr<T> ObjectFactory(void)
{
	return make_shared<T>();
}

template<typename T>
struct FactoryHelper
{
	Type::Factory GetFactory(void)
	{
		return ObjectFactory<T>;
	}
};

#define REGISTER_TYPE(type) \
	namespace { \
		void RegisterType(void) \
		{ \
			icinga::Type *t = new TypeImpl<type>(); \
			t->SetFactory(FactoryHelper<type>().GetFactory()); \
			icinga::Type::Register(t); \
		} \
		\
		INITIALIZE_ONCE(RegisterType); \
	}

}

#endif /* TYPE_H */
