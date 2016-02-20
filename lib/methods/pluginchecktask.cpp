/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginCheck,  &PluginCheckTask::ScriptFunc);

void PluginCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("command", commandObj));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	PluginUtility::ExecuteCommand(commandObj, checkable, checkable->GetLastCheckResult(),
	    resolvers, resolvedMacros, useResolvedMacros,
	    boost::bind(&PluginCheckTask::ProcessFinishedHandler, checkable, cr, _1, _2));
}

void PluginCheckTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const Value& commandLine, const ProcessResult& pr)
{
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
