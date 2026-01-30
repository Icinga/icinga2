// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/loader.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"

using namespace icinga;

boost::thread_specific_ptr<Loader::DeferredInitializerPriorityQueue>& Loader::GetDeferredInitializers()
{
	static boost::thread_specific_ptr<DeferredInitializerPriorityQueue> initializers;
	return initializers;
}

void Loader::ExecuteDeferredInitializers()
{
	auto& initializers = GetDeferredInitializers();
	if (!initializers.get())
		return;

	while (!initializers->empty()) {
		DeferredInitializer initializer = initializers->top();
		initializers->pop();
		initializer();
	}
}

void Loader::AddDeferredInitializer(const std::function<void()>& callback, InitializePriority priority)
{
	auto& initializers = GetDeferredInitializers();
	if (!initializers.get()) {
		initializers.reset(new Loader::DeferredInitializerPriorityQueue());
	}

	initializers->push(DeferredInitializer(callback, priority));
}
