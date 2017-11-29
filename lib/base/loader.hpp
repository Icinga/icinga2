/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
	DeferredInitializer(const std::function<void (void)>& callback, int priority)
	    : m_Callback(callback), m_Priority(priority)
	{ }

	inline bool operator<(const DeferredInitializer& other) const
	{
		return m_Priority < other.m_Priority;
	}

	inline void operator()(void)
	{
		m_Callback();
	}

private:
	std::function<void (void)> m_Callback;
	int m_Priority;
};

/**
 * Loader helper functions.
 *
 * @ingroup base
 */
class I2_BASE_API Loader
{
public:
	static void LoadExtensionLibrary(const String& library);

	static void AddDeferredInitializer(const std::function<void(void)>& callback, int priority = 0);
	static void ExecuteDeferredInitializers(void);

private:
	Loader(void);

	static boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> >& GetDeferredInitializers(void);
};

}

#endif /* LOADER_H */
