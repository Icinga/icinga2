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

#ifndef REGISTRY_H
#define REGISTRY_H

#include "base/i2-base.h"
#include "base/singleton.h"
#include "base/qstring.h"
#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/signals2.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

namespace icinga
{

/**
 * A registry.
 *
 * @ingroup base
 */
template<typename U, typename T>
class Registry
{
public:
	typedef std::map<String, T, string_iless> ItemMap;

	static Registry<U, T> *GetInstance(void)
	{
		return Singleton<Registry<U, T> >::GetInstance();
	}

	void Register(const String& name, const T& item)
	{
		bool old_item = false;

		{
			boost::mutex::scoped_lock lock(m_Mutex);

			if (m_Items.erase(name) > 0)
				old_item = true;

			m_Items[name] = item;
		}

		if (old_item)
			OnUnregistered(name);

		OnRegistered(name, item);
	}

	void Unregister(const String& name)
	{
		size_t erased;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			erased = m_Items.erase(name);
		}

		if (erased > 0)
			OnUnregistered(name);
	}

	void Clear(void)
	{
		typename Registry<U, T>::ItemMap items;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			items = m_Items;
		}

		String name;
		BOOST_FOREACH(boost::tie(name, boost::tuples::ignore), items) {
			OnUnregistered(name);
		}

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			m_Items.clear();
		}
	}

	T GetItem(const String& name) const
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		typename ItemMap::const_iterator it;
		it = m_Items.find(name);

		if (it == m_Items.end())
			return T();

		return it->second;
	}

	ItemMap GetItems(void) const
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		return m_Items; /* Makes a copy of the map. */
	}

	boost::signals2::signal<void (const String&, const T&)> OnRegistered;
	boost::signals2::signal<void (const String&)> OnUnregistered;

private:
	mutable boost::mutex m_Mutex;
	typename Registry<U, T>::ItemMap m_Items;
};

}

#endif /* REGISTRY_H */
