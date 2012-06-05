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

template<typename TKey = string, typename TValue = Object::Ptr>
class I2_DYN_API ObjectMap : public Object
{
public:
	typedef shared_ptr<ObjectMap<TKey, TValue> > Ptr;
	typedef weak_ptr<ObjectMap<TKey, TValue > > WeakPtr;

	typedef typename multimap<TKey, TValue>::iterator Iterator;
	typedef pair<Iterator, Iterator> Range;

	ObjectMap(const typename ObjectSet<TValue>::Ptr& parent,
	    function<bool (const TValue&, TKey *key)> keygetter)
		: m_Parent(parent), m_KeyGetter(keygetter)
	{ 
		assert(m_Parent);
		assert(m_KeyGetter);
	}

	void Start(void)
	{
		m_Parent->OnObjectAdded += bind_weak(&ObjectMap::ObjectAddedHandler, shared_from_this());
        	m_Parent->OnObjectCommitted += bind_weak(&ObjectMap::ObjectCommittedHandler, shared_from_this());
	        m_Parent->OnObjectRemoved += bind_weak(&ObjectMap::ObjectRemovedHandler, shared_from_this());

        	for (typename ObjectSet<TValue>::Iterator it = m_Parent->Begin(); it != m_Parent->End(); it++)
	                AddObject(*it);
	}

	Range GetRange(TKey key)
	{
		return m_Objects.equal_range(key);
	}

private:
	multimap<TKey, TValue> m_Objects;
	typename ObjectSet<TValue>::Ptr m_Parent;
	function<bool (const TValue&, TKey *key)> m_KeyGetter;

	void AddObject(const TValue& object)
	{
		TKey key;
		if (!m_KeyGetter(object, &key))
			return;

		m_Objects.insert(make_pair(key, object));
	}

	void RemoveObject(const TValue& object)
	{
		TKey key;
		if (!m_KeyGetter(object, &key))
        	        return;

	        pair<Iterator, Iterator> range = GetRange(key);

        	for (Iterator i = range.first; i != range.second; i++) {
                	if (i->second == object) {
	                        m_Objects.erase(i);
        	                break;
                	}
        	}
	}

	void CheckObject(const TValue& object)
	{
		RemoveObject(object);
		AddObject(object);
	}

	int ObjectAddedHandler(const ObjectSetEventArgs<TValue>& ea)
	{
		AddObject(ea.Target);

		return 0;
	}

	int ObjectCommittedHandler(const ObjectSetEventArgs<TValue>& ea)
	{
		CheckObject(ea.Target);

		return 0;
	}

	int ObjectRemovedHandler(const ObjectSetEventArgs<TValue>& ea)
	{
		RemoveObject(ea.Target);

		return 0;
	}
};

}

#endif /* OBJECTMAP_H */
