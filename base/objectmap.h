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
class ObjectMap : public Object
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
		m_Parent->OnObjectAdded.connect(boost::bind(&ObjectMap::ObjectAddedHandler, this, _2));
		m_Parent->OnObjectCommitted.connect(boost::bind(&ObjectMap::ObjectCommittedHandler, this, _2));
		m_Parent->OnObjectRemoved.connect(boost::bind(&ObjectMap::ObjectRemovedHandler, this, _2));

		BOOST_FOREACH(const TValue& object, m_Parent) {
			AddObject(object);
		}
	}

	Range GetRange(TKey key)
	{
		return m_Objects.equal_range(key);
	}

	void ForeachObject(TKey key, function<void (const typename ObjectMap<TValue>::Ptr, const TValue&)> callback)
	{
		Range range = GetRange(key);

		BOOST_FOREACH(const TValue& object, range) {
			callback(GetSelf(), object);
		}
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

	void ObjectAddedHandler(const TValue& object)
	{
		AddObject(object);
	}

	void ObjectCommittedHandler(const TValue& object)
	{
		CheckObject(object);
	}

	void ObjectRemovedHandler(const TValue& object)
	{
		RemoveObject(object);
	}
};

template<typename TKey, typename TValue>
typename ObjectMap<TKey, TValue>::Iterator range_begin(typename ObjectMap<TKey, TValue>::Ptr x)
{
	return x->Begin();
}

template <typename TKey, typename TValue>
typename ObjectSet<TValue>::Iterator range_end(typename ObjectMap<TKey, TValue>::Ptr x)
{
	return x->End();
}

}

namespace boost
{

template<typename TKey, typename TValue>
struct range_mutable_iterator<shared_ptr<icinga::ObjectMap<TKey, TValue> > >
{
	typedef shared_ptr<icinga::ObjectMap<TKey, TValue> > objtype;
	typedef typename objtype::Iterator type;
};

template<typename TKey, typename TValue>
struct range_const_iterator<shared_ptr<icinga::ObjectMap<TKey, TValue> > >
{
	typedef shared_ptr<icinga::ObjectMap<TKey, TValue> > objtype;
	typedef typename objtype::Iterator type;
};

}

#endif /* OBJECTMAP_H */
