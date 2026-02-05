// SPDX-FileCopyrightText: 2023 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Executes checks via Icinga for Windows API.
 *
 * @ingroup methods
 */
class IfwApiCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	IfwApiCheckTask();
};

}
