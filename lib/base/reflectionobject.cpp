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

#include "base/reflectionobject.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

Dictionary::Ptr ReflectionObject::Serialize(int attributeTypes) const
{
	Dictionary::Ptr update = boost::make_shared<Dictionary>();

	for (int i = 0; i < GetFieldCount(); i++) {
		ReflectionField field = GetFieldInfo(i);

		if ((field.Attributes & attributeTypes) == 0)
			continue;

		update->Set(field.Name, GetField(i));
	}

	return update;
}

void ReflectionObject::Deserialize(const Dictionary::Ptr& update, int attributeTypes)
{
	for (int i = 0; i < GetFieldCount(); i++) {
		ReflectionField field = GetFieldInfo(i);

		if ((field.Attributes & attributeTypes) == 0)
			continue;

		if (!update->Contains(field.Name))
			continue;

		SetField(i, update->Get(field.Name));
	}
}
