// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

REGISTER_FUNCTION_NONCONST(Internal, ExceptionCheck, &ExceptionCheckTask::ScriptFunc, "checkable:cr:producer:resolvedMacros:useResolvedMacros");

void ExceptionCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr&, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	ScriptError scriptError = ScriptError("Test") << boost::errinfo_api_function("Test");

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = scriptError.what();
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = 3;

		Checkable::ExecuteCommandProcessFinishedHandler("", pr);
	} else {
		BOOST_THROW_EXCEPTION(scriptError);
	}
}
