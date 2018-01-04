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

#ifndef LOADER_H
#define LOADER_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <boost/thread/tss.hpp>
#include <queue>

namespace icinga
{

struct DeferredInitializer
{
public:
	DeferredInitializer(std::function<void ()> callback, int priority)
		: m_Callback(std::move(callback)), m_Priority(priority)
	{ }

	bool operator<(const DeferredInitializer& other) const
	{
		return m_Priority < other.m_Priority;
	}

	void operator()()
	{
		m_Callback();
	}

private:
	std::function<void ()> m_Callback;
	int m_Priority;
};

/**
 * Loader helper functions.
 *
 * @ingroup base
 */
class Loader
{
public:
	static void AddDeferredInitializer(const std::function<void ()>& callback, int priority = 0);
	static void ExecuteDeferredInitializers();

private:
	Loader();

	static boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> >& GetDeferredInitializers();
};

}

#endif /* LOADER_H */
