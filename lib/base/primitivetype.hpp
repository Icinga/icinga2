/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef PRIMITIVETYPE_H
#define PRIMITIVETYPE_H

#include "base/i2-base.hpp"
#include "base/type.hpp"
#include "base/initialize.hpp"

namespace icinga
{

class PrimitiveType final : public Type
{
public:
	PrimitiveType(const String& name, const String& base, const ObjectFactory& factory = ObjectFactory());

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

#define REGISTER_BUILTIN_TYPE(type, prototype)					\
	INITIALIZE_ONCE_WITH_PRIORITY([]() {					\
		icinga::Type::Ptr t = new PrimitiveType(#type, "None"); 	\
		t->SetPrototype(prototype);					\
		icinga::Type::Register(t);					\
	}, 15)

#define REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, factory)		\
	INITIALIZE_ONCE_WITH_PRIORITY([]() {					\
		icinga::Type::Ptr t = new PrimitiveType(#type, #base, factory);	\
		t->SetPrototype(prototype);					\
		icinga::Type::Register(t);					\
		type::TypeInstance = t;						\
	}, 15);									\
	DEFINE_TYPE_INSTANCE(type)

#define REGISTER_PRIMITIVE_TYPE(type, base, prototype)				\
	REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, DefaultObjectFactory<type>)

#define REGISTER_PRIMITIVE_TYPE_VA(type, base, prototype)				\
	REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, DefaultObjectFactoryVA<type>)

#define REGISTER_PRIMITIVE_TYPE_NOINST(type, base, prototype)			\
	REGISTER_PRIMITIVE_TYPE_FACTORY(type, base, prototype, nullptr)

}

#endif /* PRIMITIVETYPE_H */
