/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/function.hpp"
#include "base/logger.hpp"
#include "methods/nulleventtask.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, NullEvent, &NullEventTask::ScriptFunc, "checkable:resolvedMacros:useResolvedMacros");

void NullEventTask::ScriptFunc(const Checkable::Ptr& checkable, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
}
