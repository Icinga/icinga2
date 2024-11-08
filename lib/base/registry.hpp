/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef REGISTRY_H
#define REGISTRY_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <boost/signals2.hpp>
#include <map>
#include <mutex>

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

	void Register(const String& name, const T& item)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		RegisterInternal(name, item, lock);
	}

	T GetItem(const String& name) const
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		auto it = m_Items.find(name);

		if (it == m_Items.end())
			return T();

		return it->second;
	}

	ItemMap GetItems() const
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		return m_Items; /* Makes a copy of the map. */
	}

	boost::signals2::signal<void (const String&, const T&)> OnRegistered;

private:
	mutable std::mutex m_Mutex;
	typename Registry<U, T>::ItemMap m_Items;

	void RegisterInternal(const String& name, const T& item, std::unique_lock<std::mutex>& lock)
	{
		m_Items[name] = item;

		lock.unlock();

		OnRegistered(name, item);
	}
};

}

#endif /* REGISTRY_H */
