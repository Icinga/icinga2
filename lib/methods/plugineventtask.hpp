/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef PLUGINEVENTTASK_H
#define PLUGINEVENTTASK_H

#include "base/process.hpp"
#include "icinga/service.hpp"
#include "methods/i2-methods.hpp"

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
