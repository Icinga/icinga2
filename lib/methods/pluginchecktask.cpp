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

#include "methods/pluginchecktask.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, PluginCheck,  &PluginCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void PluginCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
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

	PluginUtility::ExecuteCommand(commandObj, checkable, checkable->GetLastCheckResult(),
	    resolvers, resolvedMacros, useResolvedMacros,
	    std::bind(&PluginCheckTask::ProcessFinishedHandler, checkable, cr, _1, _2));

	if (!resolvedMacros || useResolvedMacros)
		Checkable::IncreasePendingChecks();
}

void PluginCheckTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const Value& commandLine, const ProcessResult& pr)
{
	Checkable::DecreasePendingChecks();

	if (pr.ExitStatus > 3) {
		Process::Arguments parguments = Process::PrepareCommand(commandLine);
		Log(LogWarning, "PluginCheckTask")
		    << "Check command for object '" << checkable->GetName() << "' (PID: " << pr.PID
		    << ", arguments: " << Process::PrettyPrintArguments(parguments) << ") terminated with exit code "
		    << pr.ExitStatus << ", output: " << pr.Output;
	}

	String output = pr.Output.Trim();

	std::pair<String, String> co = PluginUtility::ParseCheckOutput(output);
	cr->SetCommand(commandLine);
	cr->SetOutput(co.first);
	cr->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
	cr->SetState(PluginUtility::ExitStatusToState(pr.ExitStatus));
	cr->SetExitStatus(pr.ExitStatus);
	cr->SetExecutionStart(pr.ExecutionStart);
	cr->SetExecutionEnd(pr.ExecutionEnd);

	checkable->ProcessCheckResult(cr);
}
