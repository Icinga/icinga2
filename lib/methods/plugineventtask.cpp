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

#include "methods/plugineventtask.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, PluginEvent, &PluginEventTask::ScriptFunc, "checkable:resolvedMacros:useResolvedMacros");

void PluginEventTask::ScriptFunc(const Checkable::Ptr& checkable,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	EventCommand::Ptr commandObj = checkable->GetEventCommand();

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
	    std::bind(&PluginEventTask::ProcessFinishedHandler, checkable, _1, _2));
}

void PluginEventTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const Value& commandLine, const ProcessResult& pr)
{
	if (pr.ExitStatus != 0) {
		Process::Arguments parguments = Process::PrepareCommand(commandLine);
		Log(LogNotice, "PluginEventTask")
		    << "Event command for object '" << checkable->GetName() << "' (PID: " << pr.PID
		    << ", arguments: " << Process::PrettyPrintArguments(parguments) << ") terminated with exit code "
		    << pr.ExitStatus << ", output: " << pr.Output;
	}
}
