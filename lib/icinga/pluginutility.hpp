/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef PLUGINUTILITY_H
#define PLUGINUTILITY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkable.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include <vector>

namespace icinga
{

struct ProcessResult;

/**
 * Utility functions for plugin-based checks.
 *
 * @ingroup icinga
 */
class PluginUtility
{
public:
	static void ExecuteCommand(const Command::Ptr& commandObj, const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, const MacroProcessor::ResolverList& macroResolvers,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros, int timeout,
		const std::function<void(const Value& commandLine, const ProcessResult&)>& callback = std::function<void(const Value& commandLine, const ProcessResult&)>(),
		const Array::Ptr& safeToTruncate = nullptr);

	static ServiceState ExitStatusToState(int exitStatus);
	static std::pair<String, String> ParseCheckOutput(const String& output);

	static Array::Ptr SplitPerfdata(const String& perfdata);
	static String FormatPerfdata(const Array::Ptr& perfdata, bool normalize = false);

private:
	PluginUtility();
};

}

#endif /* PLUGINUTILITY_H */
