// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LOADER_H
#define LOADER_H

#include "base/i2-base.hpp"
#include "base/initialize.hpp"
#include "base/string.hpp"
#include <boost/thread/tss.hpp>
#include <queue>

namespace icinga
{

struct DeferredInitializer
{
public:
	DeferredInitializer(std::function<void ()> callback, InitializePriority priority)
		: m_Callback(std::move(callback)), m_Priority(priority)
	{ }

	bool operator>(const DeferredInitializer& other) const
	{
		return m_Priority > other.m_Priority;
	}

	void operator()()
	{
		m_Callback();
	}

private:
	std::function<void ()> m_Callback;
	InitializePriority m_Priority;
};

/**
 * Loader helper functions.
 *
 * @ingroup base
 */
class Loader
{
public:
	static void AddDeferredInitializer(const std::function<void ()>& callback, InitializePriority priority = InitializePriority::Default);
	static void ExecuteDeferredInitializers();

private:
	Loader();

	// Deferred initializers are run in the order of the definition of their enum values.
	// Therefore, initializers that should be run first have lower enum values and
	// the order of the std::priority_queue has to be reversed using std::greater.
	using DeferredInitializerPriorityQueue = std::priority_queue<DeferredInitializer, std::vector<DeferredInitializer>, std::greater<>>;

	static boost::thread_specific_ptr<DeferredInitializerPriorityQueue>& GetDeferredInitializers();
};

}

#endif /* LOADER_H */
