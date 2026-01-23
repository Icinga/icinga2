/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/nulleventtask.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, NullEvent, &NullEventTask::ScriptFunc, "checkable:resolvedMacros:useResolvedMacros");

void NullEventTask::ScriptFunc(const Checkable::Ptr& checkable, [[maybe_unused]] const Dictionary::Ptr& resolvedMacros, [[maybe_unused]] bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = "";
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = 0;

		Checkable::ExecuteCommandProcessFinishedHandler("", pr);
	}
}
