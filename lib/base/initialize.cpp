/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/initialize.hpp"
#include "base/loader.hpp"

using namespace icinga;

bool icinga::InitializeOnceHelper(const std::function<void()>& func, InitializePriority priority)
{
	Loader::AddDeferredInitializer(func, priority);
	return true;
}

