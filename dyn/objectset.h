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

#ifndef OBJECTSET_H
#define OBJECTSET_H

namespace icinga
{

struct ObjectSetEventArgs : public EventArgs
{
	Object::Ptr Target;
};

typedef function<bool (const Object::Ptr&)> ObjectPredicate;

class I2_DYN_API ObjectSet : public Object
{
public:
	typedef shared_ptr<ObjectSet> Ptr;
	typedef weak_ptr<ObjectSet> WeakPtr;

	typedef set<Object::Ptr>::iterator Iterator;

	ObjectSet(void);
	ObjectSet(const ObjectSet::Ptr& parent, ObjectPredicate filter);

	void Start(void);

	void AddObject(const Object::Ptr& object);
	void RemoveObject(const Object::Ptr& object);
	bool Contains(const Object::Ptr& object) const;

	void CheckObject(const Object::Ptr& object);

	Observable<ObjectSetEventArgs> OnObjectAdded;
	Observable<ObjectSetEventArgs> OnObjectCommitted;
	Observable<ObjectSetEventArgs> OnObjectRemoved;

	Iterator Begin(void);
	Iterator End(void);

	static ObjectSet::Ptr GetAllObjects(void);

private:
	set<Object::Ptr> m_Objects;

	ObjectSet::Ptr m_Parent;
	ObjectPredicate m_Filter;

	int ObjectAddedOrCommittedHandler(const ObjectSetEventArgs& ea);
	int ObjectRemovedHandler(const ObjectSetEventArgs& ea);
};

}

#endif /* OBJECTSET_H */