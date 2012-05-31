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

#ifndef OBJECTMAP_H
#define OBJECTMAP_H

namespace icinga
{

typedef function<bool (const Object::Ptr&, string *key)> ObjectKeyGetter;

class I2_DYN_API ObjectMap : public Object
{
public:
	typedef shared_ptr<ObjectMap> Ptr;
	typedef weak_ptr<ObjectMap> WeakPtr;

	typedef multimap<string, Object::Ptr>::iterator Iterator;
	typedef pair<Iterator, Iterator> Range;

	ObjectMap(const ObjectSet::Ptr& parent, ObjectKeyGetter keygetter);

	void Start(void);

	Range GetRange(string key);

private:
	multimap<string, Object::Ptr> m_Objects;
	ObjectSet::Ptr m_Parent;
	ObjectKeyGetter m_KeyGetter;

	void AddObject(const Object::Ptr& object);
	void RemoveObject(const Object::Ptr& object);
	void CheckObject(const Object::Ptr& object);

	int ObjectAddedHandler(const ObjectSetEventArgs& ea);
	int ObjectCommittedHandler(const ObjectSetEventArgs& ea);
	int ObjectRemovedHandler(const ObjectSetEventArgs& ea);
};

}

#endif /* OBJECTMAP_H */
