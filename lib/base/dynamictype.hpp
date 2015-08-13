/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef DYNAMICTYPE_H
#define DYNAMICTYPE_H

#include "base/i2-base.hpp"
#include "base/dynamicobject.hpp"
#include "base/objectlock.hpp"
#include <map>
# include <boost/iterator/iterator_facade.hpp>

namespace icinga
{

template<typename T>
class DynamicTypeIterator;

class I2_BASE_API DynamicType : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DynamicType);

	DynamicType(const String& name);

	String GetName(void) const;

	static DynamicType::Ptr GetByName(const String& name);

	DynamicObject::Ptr GetObject(const String& name) const;

	void RegisterObject(const DynamicObject::Ptr& object);
	void UnregisterObject(const DynamicObject::Ptr& object);

	static std::vector<DynamicType::Ptr> GetTypes(void);
	std::pair<DynamicTypeIterator<DynamicObject>, DynamicTypeIterator<DynamicObject> > GetObjects(void);

	template<typename T>
	static std::pair<DynamicTypeIterator<T>, DynamicTypeIterator<T> > GetObjectsByType(void)
	{
		DynamicType::Ptr type = GetByName(T::GetTypeName());
		return std::make_pair(
		    DynamicTypeIterator<T>(type, 0),
		    DynamicTypeIterator<T>(type, UINT_MAX)
		);
	}

private:
	template<typename T> friend class DynamicTypeIterator;

	String m_Name;

	typedef std::map<String, DynamicObject::Ptr> ObjectMap;
	typedef std::vector<DynamicObject::Ptr> ObjectVector;

	ObjectMap m_ObjectMap;
	ObjectVector m_ObjectVector;

	typedef std::map<String, DynamicType::Ptr> TypeMap;
	typedef std::vector<DynamicType::Ptr> TypeVector;

	static TypeMap& InternalGetTypeMap(void);
	static TypeVector& InternalGetTypeVector(void);
	static boost::mutex& GetStaticMutex(void);
};

template<typename T>
class DynamicTypeIterator : public boost::iterator_facade<DynamicTypeIterator<T>, const intrusive_ptr<T>, boost::forward_traversal_tag>
{
public:
	DynamicTypeIterator(const DynamicType::Ptr& type, int index)
		: m_Type(type), m_Index(index)
	{ }

private:
	friend class boost::iterator_core_access;

	DynamicType::Ptr m_Type;
	DynamicType::ObjectVector::size_type m_Index;
	mutable intrusive_ptr<T> m_Current;

	void increment(void)
	{
		m_Index++;
	}

	void decrement(void)
	{
		m_Index--;
	}

	void advance(int n)
	{
		m_Index += n;
	}

	bool equal(const DynamicTypeIterator<T>& other) const
	{
		ASSERT(other.m_Type == m_Type);

		{
			ObjectLock olock(m_Type);

			if ((other.m_Index == UINT_MAX || other.m_Index >= other.m_Type->m_ObjectVector.size()) &&
			    (m_Index == UINT_MAX || m_Index >= m_Type->m_ObjectVector.size()))
				return true;
		}

		return (other.m_Index == m_Index);
	}

	const intrusive_ptr<T>& dereference(void) const
	{
		ObjectLock olock(m_Type);
		m_Current = static_pointer_cast<T>(*(m_Type->m_ObjectVector.begin() + m_Index));
		return m_Current;
	}
};

}

#endif /* DYNAMICTYPE_H */
