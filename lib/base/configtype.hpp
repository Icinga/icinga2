/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef CONFIGTYPE_H
#define CONFIGTYPE_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/type.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

class ConfigObject;

template<typename T>
class ConfigTypeIterator;

class I2_BASE_API ConfigType
{
public:
	virtual ~ConfigType(void);

	intrusive_ptr<ConfigObject> GetObject(const String& name) const;

	void RegisterObject(const intrusive_ptr<ConfigObject>& object);
	void UnregisterObject(const intrusive_ptr<ConfigObject>& object);

	std::pair<ConfigTypeIterator<ConfigObject>, ConfigTypeIterator<ConfigObject> > GetObjects(void);

	template<typename T>
	static std::pair<ConfigTypeIterator<T>, ConfigTypeIterator<T> > GetObjectsByType(void)
	{
		Type::Ptr type = T::TypeInstance;
		return std::make_pair(
		    ConfigTypeIterator<T>(type, 0),
		    ConfigTypeIterator<T>(type, UINT_MAX)
		);
	}

private:
	template<typename T> friend class ConfigTypeIterator;

	typedef std::map<String, intrusive_ptr<ConfigObject> > ObjectMap;
	typedef std::vector<intrusive_ptr<ConfigObject> > ObjectVector;

	mutable boost::mutex m_Mutex;
	ObjectMap m_ObjectMap;
	ObjectVector m_ObjectVector;
};

template<typename T>
class ConfigTypeIterator : public boost::iterator_facade<ConfigTypeIterator<T>, const intrusive_ptr<T>, boost::forward_traversal_tag>
{
public:
	ConfigTypeIterator(const Type::Ptr& type, int index)
		: m_Type(type), m_ConfigType(dynamic_cast<ConfigType *>(type.get())), m_Index(index)
	{
		ASSERT(m_ConfigType);
	}

private:
	friend class boost::iterator_core_access;

	Type::Ptr m_Type;
	ConfigType *m_ConfigType;
	ConfigType::ObjectVector::size_type m_Index;
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

	bool equal(const ConfigTypeIterator<T>& other) const
	{
		ASSERT(other.m_Type == m_Type);

		{
			boost::mutex::scoped_lock lock(m_ConfigType->m_Mutex);

			if ((other.m_Index == UINT_MAX || other.m_Index >= other.m_ConfigType->m_ObjectVector.size()) &&
			    (m_Index == UINT_MAX || m_Index >= m_ConfigType->m_ObjectVector.size()))
				return true;
		}

		return (other.m_Index == m_Index);
	}

	const intrusive_ptr<T>& dereference(void) const
	{
		boost::mutex::scoped_lock lock(m_ConfigType->m_Mutex);
		m_Current = static_pointer_cast<T>(*(m_ConfigType->m_ObjectVector.begin() + m_Index));
		return m_Current;
	}
};
}

#endif /* CONFIGTYPE_H */
