/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef REGISTRY_H
#define REGISTRY_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/signals2.hpp>
#include <map>

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
