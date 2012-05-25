/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef OBJECTSPACE_H
#define OBJECTSPACE_H

namespace icinga
{

typedef function<DynamicObject::Ptr()> DynamicObjectFactory;

class ObjectSpace : public Object
{
public:
	void RegisterClass(string name, DynamicObjectFactory factory);
	void UnregisterClass(string name);

	Dictionary::Ptr SerializeObject(DynamicObject::Ptr object);
	DynamicObject::Ptr UnserializeObject(Dictionary::Ptr serializedObject);

	vector<DynamicObject::Ptr> FindObjects(function<bool (DynamicObject::Ptr)> predicate);

private:
	map<string, DynamicObjectFactory> m_Classes;
	set<DynamicObject::Ptr> m_Objects;

	void RegisterObject(DynamicObject::Ptr object);
	void UnregisterObject(DynamicObject::Ptr object);
};

}

#endif /* OBJECTSPACE_H */
