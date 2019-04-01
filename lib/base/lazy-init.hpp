/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef LAZY_INIT
#define LAZY_INIT

#include <atomic>
#include <functional>
#include <mutex>
#include <utility>

namespace icinga
{

/**
 * Lazy object initialization abstraction inspired from
 * <https://docs.microsoft.com/en-us/dotnet/api/system.lazy-1?view=netframework-4.7.2>.
 *
 * @ingroup base
 */
template<class T>
class LazyInit
{
public:
	inline
	LazyInit(std::function<T()> initializer = []() { return T(); }) : m_Initializer(std::move(initializer))
	{
		m_Underlying.store(nullptr, std::memory_order_release);
	}

	LazyInit(const LazyInit&) = delete;
	LazyInit(LazyInit&&) = delete;
	LazyInit& operator=(const LazyInit&) = delete;
	LazyInit& operator=(LazyInit&&) = delete;

	inline
	~LazyInit()
	{
		auto ptr (m_Underlying.load(std::memory_order_acquire));

		if (ptr != nullptr) {
			delete ptr;
		}
	}

	inline
	T& Get()
	{
		auto ptr (m_Underlying.load(std::memory_order_acquire));

		if (ptr == nullptr) {
			std::unique_lock<std::mutex> lock (m_Mutex);

			ptr = m_Underlying.load(std::memory_order_acquire);

			if (ptr == nullptr) {
				ptr = new T(m_Initializer());
				m_Underlying.store(ptr, std::memory_order_release);
			}
		}

		return *ptr;
	}

private:
	std::function<T()> m_Initializer;
	std::mutex m_Mutex;
	std::atomic<T*> m_Underlying;
};

}

#endif /* LAZY_INIT */
