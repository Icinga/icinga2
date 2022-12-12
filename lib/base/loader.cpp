/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/loader.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> >& Loader::GetDeferredInitializers()
{
	static boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> > initializers;
	return initializers;
}

void Loader::ExecuteDeferredInitializers()
{
	if (!GetDeferredInitializers().get())
		return;

	while (!GetDeferredInitializers().get()->empty()) {
		DeferredInitializer initializer = GetDeferredInitializers().get()->top();
		GetDeferredInitializers().get()->pop();
		initializer();
	}
}

void Loader::AddDeferredInitializer(const std::function<void()>& callback, InitializePriority priority)
{
	if (!GetDeferredInitializers().get())
		GetDeferredInitializers().reset(new std::priority_queue<DeferredInitializer>());

	GetDeferredInitializers().get()->push(DeferredInitializer(callback, priority));
}

