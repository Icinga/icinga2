// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PLUGINCHECKTASK_H
#define PLUGINCHECKTASK_H

#include "methods/i2-methods.hpp"
#include "base/process.hpp"
#include "icinga/service.hpp"

namespace icinga
{

/**
 * Implements service checks based on external plugins.
 *
 * @ingroup methods
 */
class PluginCheckTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	PluginCheckTask();

	static void ProcessFinishedHandler(const Checkable::Ptr& service, const CheckResult::Ptr& cr,
		const WaitGroup::Ptr& producer, const Value& commandLine, const ProcessResult& pr);
};

}

#endif /* PLUGINCHECKTASK_H */
