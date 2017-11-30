/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/dummychecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/pluginutility.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, DummyCheck, &DummyCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void DummyCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	int dummyState = MacroProcessor::ResolveMacros("$dummy_state$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String dummyText = MacroProcessor::ResolveMacros("$dummy_text$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	/* Parse output and performance data. */
	std::pair<String, String> co = PluginUtility::ParseCheckOutput(dummyText);

	double now = Utility::GetTime();

	cr->SetOutput(co.first);
	cr->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
	cr->SetState(PluginUtility::ExitStatusToState(dummyState));
	cr->SetExitStatus(dummyState);
	cr->SetExecutionStart(now);
	cr->SetExecutionEnd(now);

	checkable->ProcessCheckResult(cr);
}
