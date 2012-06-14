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

template<typename TValue>
struct ObjectSetEventArgs : public EventArgs
{
	TValue Target;
};

template<typename TValue>
class ObjectSet : public Object
{
public:
	typedef shared_ptr<ObjectSet<TValue> > Ptr;
	typedef weak_ptr<ObjectSet<TValue> > WeakPtr;

	typedef typename set<TValue>::iterator Iterator;

	ObjectSet(void)
		: m_Parent(), m_Predicate()
	{ }

	ObjectSet(const typename ObjectSet<TValue>::Ptr& parent, function<bool(const TValue&)> predicate)
		: m_Parent(parent), m_Predicate(predicate)
	{ }

	void Start(void)
	{
		if (m_Parent) {
			m_Parent->OnObjectAdded.connect(bind(&ObjectSet::ObjectAddedOrCommittedHandler, this, _1));
			m_Parent->OnObjectCommitted.connect(bind(&ObjectSet::ObjectAddedOrCommittedHandler, this, _1));
			m_Parent->OnObjectRemoved.connect(bind(&ObjectSet::ObjectRemovedHandler, this, _1));

			for (ObjectSet::Iterator it = m_Parent->Begin(); it != m_Parent->End(); it++)
				CheckObject(*it);
	        }


	}

	void AddObject(const TValue& object)
	{
		m_Objects.insert(object);

		ObjectSetEventArgs<TValue> ea;
		ea.Source = shared_from_this();
		ea.Target = object;
		OnObjectAdded(ea);
	}

	void RemoveObject(const TValue& object)
	{
		ObjectSet::Iterator it = m_Objects.find(object);

		if (it != m_Objects.end()) {
			m_Objects.erase(it);

			ObjectSetEventArgs<TValue> ea;
			ea.Source = shared_from_this();
			ea.Target = object;
			OnObjectRemoved(ea);
		}
	}

	bool Contains(const TValue& object) const
	{
		ObjectSet::Iterator it = m_Objects.find(object);

		return !(it == m_Objects.end());
	}

	void CheckObject(const TValue& object)
	{
		if (m_Predicate && !m_Predicate(object)) {
			RemoveObject(object);
		} else {
			if (!Contains(object)) {
				AddObject(object);
			} else {
				ObjectSetEventArgs<TValue> ea;
				ea.Source = shared_from_this();
				ea.Target = object;
				OnObjectCommitted(ea);
			}
		}
	}

	boost::signal<void (const ObjectSetEventArgs<TValue>&)> OnObjectAdded;
	boost::signal<void (const ObjectSetEventArgs<TValue>&)> OnObjectCommitted;
	boost::signal<void (const ObjectSetEventArgs<TValue>&)> OnObjectRemoved;

	Iterator Begin(void)
	{
		return m_Objects.begin();
	}

	Iterator End(void)
	{
		return m_Objects.end();
	}

	void ForeachObject(function<int (const ObjectSetEventArgs<TValue>&)> callback)
	{
		ObjectSetEventArgs<TValue> ea;
		ea.Source = shared_from_this();

		for (Iterator it = Begin(); it != End(); it++) {
			ea.Target(*it);
			callback(ea);
		}
	}

private:
	set<TValue> m_Objects;

	typename ObjectSet<TValue>::Ptr m_Parent;
	function<bool (const TValue&)> m_Predicate;

	int ObjectAddedOrCommittedHandler(const ObjectSetEventArgs<TValue>& ea)
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

#endif /* OBJECTSET_H */
