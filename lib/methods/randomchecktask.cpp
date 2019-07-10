/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/randomchecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, RandomCheck, &RandomCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void RandomCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	double now = Utility::GetTime();
	double uptime = now - Application::GetStartTime();

	String output = "Hello from " + IcingaApplication::GetInstance()->GetNodeName()
		+ ". Icinga 2 has been running for " + Utility::FormatDuration(uptime)
		+ ". Version: " + Application::GetAppVersion();

	cr->SetOutput(output);

	double random = Utility::Random() % 1000;

	cr->SetPerformanceData(new Array({
		new PerfdataValue("time", now),
		new PerfdataValue("value", random),
		new PerfdataValue("value_1m", random * 0.9),
		new PerfdataValue("value_5m", random * 0.8),
		new PerfdataValue("uptime", uptime),
	}));

	cr->SetState(static_cast<ServiceState>(Utility::Random() % 4));

	CheckCommand::Ptr command = checkable->GetCheckCommand();
	cr->SetCommand(command->GetName());

	checkable->ProcessCheckResult(cr);
}
