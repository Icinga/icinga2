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

#ifndef REFLECTIONOBJECT_H
#define REFLECTIONOBJECT_H

#include "base/object.h"
#include "base/dictionary.h"
#include <vector>

namespace icinga
{

enum ReflectionFieldAttribute
{
	FAConfig = 1,
	FAState = 2
};

struct ReflectionField
{
	int ID;
	String Name;
	int Attributes;
	Value DefaultValue;

	ReflectionField(int id, const String& name, int attributes, const Value& default_value = Empty)
		: ID(id), Name(name), Attributes(attributes), DefaultValue(default_value)
	{ }
};

enum InvokationType
{
	ITGet,
	ITSet
};

class I2_BASE_API ReflectionObject : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ReflectionObject);

	virtual int GetFieldId(const String& name) const = 0;
	virtual ReflectionField GetFieldInfo(int id) const = 0;
	virtual int GetFieldCount(void) const = 0;
	virtual void SetField(int id, const Value& value) = 0;
	virtual Value GetField(int id) const = 0;

	Dictionary::Ptr Serialize(int attributeTypes) const;
	void Deserialize(const Dictionary::Ptr& update, int attributeTypes);
};

template<typename T>
class ReflectionObjectImpl
{
};

}

#endif /* REFLECTIONOBJECT_H */
