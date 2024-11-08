/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef REGISTRY_H
#define REGISTRY_H

#include "base/i2-base.hpp"
#include "base/atomic.hpp"
#include "base/exception.hpp"
#include "base/string.hpp"
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
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
	typedef std::unordered_map<String, T> ItemMap;

	void Register(const String& name, const T& item)
	{
		std::unique_lock lock (m_Mutex);

		if (m_Frozen) {
			BOOST_THROW_EXCEPTION(std::logic_error("Registry is read-only and must not be modified."));
		}

		m_Items[name] = item;
	}

	T GetItem(const String& name) const
	{
		auto lock (ReadLockUnlessFrozen());

		auto it = m_Items.find(name);

		if (it == m_Items.end())
			return T();

		return it->second;
	}

	ItemMap GetItems() const
	{
		auto lock (ReadLockUnlessFrozen());

		return m_Items; /* Makes a copy of the map. */
	}

	/**
	 * Freeze the registry, preventing further updates.
	 *
	 * This only prevents inserting, replacing or deleting values from the registry.
	 * This operation has no effect on objects referenced by the values, these remain mutable if they were before.
	 */
	void Freeze()
	{
		std::unique_lock lock (m_Mutex);

		m_Frozen.store(true);
	}

private:
	mutable std::shared_mutex m_Mutex;
	Atomic<bool> m_Frozen {false};
	typename Registry<U, T>::ItemMap m_Items;

	std::shared_lock<std::shared_mutex> ReadLockUnlessFrozen() const
	{
		if (m_Frozen.load(std::memory_order_relaxed)) {
			return {};
		}

		return std::shared_lock(m_Mutex);
	}
};

}

#endif /* REGISTRY_H */
