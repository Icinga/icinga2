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

#include "base/serializer.h"
#include "base/type.h"

using namespace icinga;

Dictionary::Ptr Serializer::Serialize(const Object::Ptr& object, int attributeTypes)
{
	const Type *type = object->GetReflectionType();

	Dictionary::Ptr update = make_shared<Dictionary>();

	for (int i = 0; i < type->GetFieldCount(); i++) {
		Field field = type->GetFieldInfo(i);

		if ((field.Attributes & attributeTypes) == 0)
			continue;

		update->Set(field.Name, object->GetField(i));
	}

	return update;
}

void Serializer::Deserialize(const Object::Ptr& object, const Dictionary::Ptr& update, int attributeTypes)
{
	const Type *type = object->GetReflectionType();

	for (int i = 0; i < type->GetFieldCount(); i++) {
		Field field = type->GetFieldInfo(i);

		if ((field.Attributes & attributeTypes) == 0)
			continue;

		if (!update->Contains(field.Name))
			continue;

		object->SetField(i, update->Get(field.Name));
	}
}
