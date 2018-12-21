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

#ifndef FASTQUEUE_H
#define FASTQUEUE_H

#include "base/i2-base.hpp"
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

namespace icinga
{

/**
 * A lock-free queue.
 *
 * @ingroup base
 */
template<class T>
class FastQueue final
{
public:
	FastQueue(std::size_t capacity)
		: m_Buffer(malloc(sizeof(T) * capacity)), m_Capacity(capacity), m_First(0), m_Next(0)
	{
		if (m_Buffer == nullptr) {
			throw std::bad_alloc();
		}

		m_Size.store(0);
	}

	FastQueue(const FastQueue&) = delete;
	FastQueue(FastQueue&&) = delete;

	FastQueue& operator= (const FastQueue&) = delete;
	FastQueue& operator= (FastQueue&&) = delete;

	template<class... TArgs>
	void Push(TArgs&&... args)
	{
		while (m_Size.load() == m_Capacity) {
		}
		
		new ((T*)m_Buffer + m_Next) T(std::forward<TArgs>(args)...);

		m_Next = (m_Next + 1u) % m_Capacity;
		m_Size.fetch_add(1);
	}
	
	T Shift()
	{
		while (!m_Size.load()) {
		}

		auto item (std::move(*((T*)m_Buffer + m_First)));
		((T*)m_Buffer + m_First)->~T();

		m_First = (m_First + 1u) % m_Capacity;
		m_Size.fetch_sub(1);

		return std::move(item);
	}

	~FastQueue()
	{
		free(m_Buffer);
		VERIFY(!m_Size.load());
	}

private:
	void *m_Buffer;
	std::size_t m_Capacity, m_First, m_Next;
	std::atomic<std::size_t> m_Size;
};

}

#endif /* FASTQUEUE_H */
