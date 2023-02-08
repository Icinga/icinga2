/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#pragma once

#include <functional>
#include <map>
#include <unordered_map>
#include <utility>

namespace icinga
{

/**
 * Proxies == and std::hash to *this->Target. Useful not to duplicate hash table keys.
 *
 * @ingroup base
 */
template<class T>
struct HashProxy
{
	T const * Target;

	inline HashProxy(T const * target) : Target(target)
	{
	}

	inline bool operator==(const HashProxy& rhs) const
	{
		return *Target == *rhs.Target;
	}
};

/**
 * A tree- and hash-map. Ordered iteration combined with O(1) lookup.
 *
 * @ingroup base
 */
template<class K, class V>
class HybridMap
{
public:
	typedef typename std::map<K, V>::value_type ValueType;
	typedef typename std::map<K, V>::iterator Iterator;

	auto Contains(const K& k) const
	{
		return m_Fast.find(&k) != m_Fast.end();
	}

	auto Get(const K& k, V& v) const
	{
		auto pos (m_Fast.find(&k));

		if (pos == m_Fast.end()) {
			return false;
		}

		v = pos->second->second;
		return true;
	}

	inline auto GetLength() const
	{
		return m_Fast.size();
	}

	inline auto begin()
	{
		return m_Ordered.begin();
	}

	inline auto end()
	{
		return m_Ordered.end();
	}

	inline auto begin() const
	{
		return m_Ordered.begin();
	}

	inline auto end() const
	{
		return m_Ordered.end();
	}

	void Set(const K& k, V v)
	{
		auto pos (m_Fast.find(&k));

		if (pos == m_Fast.end()) {
			auto pos (m_Ordered.emplace(k, std::move(v)).first);

			m_Fast.emplace(&pos->first, pos);
		} else {
			pos->second->second = std::move(v);
		}
	}

	void Set(K&& k, V v)
	{
		auto pos (m_Fast.find(&k));

		if (pos == m_Fast.end()) {
			auto pos (m_Ordered.emplace(std::move(k), std::move(v)).first);

			m_Fast.emplace(&pos->first, pos);
		} else {
			pos->second->second = std::move(v);
		}
	}

	void Remove(const K& k)
	{
		auto pos (m_Fast.find(&k));

		if (pos != m_Fast.end()) {
			m_Ordered.erase(pos->second);
			m_Fast.erase(pos);
		}
	}

	void Remove(Iterator i)
	{
		m_Fast.erase(&i->first);
		m_Ordered.erase(i);
	}

	void Clear()
	{
		m_Ordered.clear();
		m_Fast.clear();
	}

private:
	std::map<K, V> m_Ordered;
	std::unordered_map<HashProxy<K>, Iterator> m_Fast;
};

}

namespace std
{
	template<class T>
	struct hash<icinga::HashProxy<T>>
	{
		inline std::size_t operator()(const icinga::HashProxy<T>& hp) const noexcept
		{
			return std::hash<T>{}(*hp.Target);
		}
	};
}
