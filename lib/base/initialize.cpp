// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/initialize.hpp"
#include "base/loader.hpp"

using namespace icinga;

bool icinga::InitializeOnceHelper(const std::function<void()>& func, InitializePriority priority)
{
	Loader::AddDeferredInitializer(func, priority);
	return true;
}
