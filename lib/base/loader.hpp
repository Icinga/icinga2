/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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

	static boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> >& GetDeferredInitializers();
};

}

#endif /* LOADER_H */
