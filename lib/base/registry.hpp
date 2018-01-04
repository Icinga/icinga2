/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/signals2.hpp>

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
	typedef std::map<String, T> ItemMap;

	void RegisterIfNew(const String& name, const T& item)
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (m_Items.find(name) != m_Items.end())
			return;

		RegisterInternal(name, item, lock);
	}

	void Register(const String& name, const T& item)
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		RegisterInternal(name, item, lock);
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

	void Clear()
	{
		typename Registry<U, T>::ItemMap items;

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			items = m_Items;
		}

		for (const auto& kv : items) {
			OnUnregistered(kv.first);
		}

		{
			boost::mutex::scoped_lock lock(m_Mutex);
			m_Items.clear();
		}
	}

	T GetItem(const String& name) const
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		auto it = m_Items.find(name);

		if (it == m_Items.end())
			return T();

		return it->second;
	}

	ItemMap GetItems() const
	{
		boost::mutex::scoped_lock lock(m_Mutex);

		return m_Items; /* Makes a copy of the map. */
	}

	boost::signals2::signal<void (const String&, const T&)> OnRegistered;
	boost::signals2::signal<void (const String&)> OnUnregistered;

private:
	mutable boost::mutex m_Mutex;
	typename Registry<U, T>::ItemMap m_Items;

	void RegisterInternal(const String& name, const T& item, boost::mutex::scoped_lock& lock)
	{
		bool old_item = false;

		if (m_Items.erase(name) > 0)
			old_item = true;

		m_Items[name] = item;

		lock.unlock();

		if (old_item)
			OnUnregistered(name);

		OnRegistered(name, item);
	}
};

}

#endif /* REGISTRY_H */
