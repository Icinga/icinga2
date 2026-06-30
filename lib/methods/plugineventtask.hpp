// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PLUGINEVENTTASK_H
#define PLUGINEVENTTASK_H

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/process.hpp"

namespace icinga
{

/**
 * Implements event handlers based on external plugins.
 *
 * @ingroup methods
 */
class PluginEventTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	PluginEventTask();

	static void ProcessFinishedHandler(const Checkable::Ptr& checkable,
		const Value& commandLine, const ProcessResult& pr);
};

}

#endif /* PLUGINEVENTTASK_H */
