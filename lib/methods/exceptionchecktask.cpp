/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/exceptionchecktask.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, ExceptionCheck, &ExceptionCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void ExceptionCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	BOOST_THROW_EXCEPTION(ScriptError("Test") << boost::errinfo_api_function("Test"));
}
