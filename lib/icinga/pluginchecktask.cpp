/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/pluginchecktask.h"
#include "icinga/checkcommand.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/scriptfunction.h"
#include "base/utility.h"
#include "base/process.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginCheck,  &PluginCheckTask::ScriptFunc);

Dictionary::Ptr PluginCheckTask::ScriptFunc(const Service::Ptr& service)
{
	CheckCommand::Ptr commandObj = service->GetCheckCommand();
	Value raw_command = commandObj->GetCommandLine();

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(commandObj);
	resolvers.push_back(IcingaApplication::GetInstance());

	Value command = MacroProcessor::ResolveMacros(raw_command, resolvers, Dictionary::Ptr(), Utility::EscapeShellCmd, commandObj->GetEscapeMacros());

	Dictionary::Ptr envMacros = boost::make_shared<Dictionary>();

	Array::Ptr export_macros = commandObj->GetExportMacros();

	if (export_macros) {
		BOOST_FOREACH(const String& macro, export_macros) {
			String value;

			if (!MacroProcessor::ResolveMacro(macro, resolvers, Dictionary::Ptr(), &value)) {
				Log(LogWarning, "icinga", "export_macros for service '" + service->GetName() + "' refers to unknown macro '" + macro + "'");
				continue;
			}

			envMacros->Set(macro, value);
		}
	}

	Process::Ptr process = boost::make_shared<Process>(Process::SplitCommand(command), envMacros);

	process->SetTimeout(commandObj->GetTimeout());

	ProcessResult pr = process->Run();

	String output = pr.Output;
	output.Trim();
	Dictionary::Ptr result = ParseCheckOutput(output);
	result->Set("command", command);
	result->Set("state", ExitStatusToState(pr.ExitStatus));
	result->Set("exit_state", pr.ExitStatus);
	result->Set("execution_start", pr.ExecutionStart);
	result->Set("execution_end", pr.ExecutionEnd);

	return result;
}

ServiceState PluginCheckTask::ExitStatusToState(int exitStatus)
{
	switch (exitStatus) {
		case 0:
			return StateOK;
		case 1:
			return StateWarning;
		case 2:
			return StateCritical;
		default:
			return StateUnknown;
	}
}

Dictionary::Ptr PluginCheckTask::ParseCheckOutput(const String& output)
{
	Dictionary::Ptr result = boost::make_shared<Dictionary>();

	String text;
	String perfdata;

	std::vector<String> lines;
	boost::algorithm::split(lines, output, boost::is_any_of("\r\n"));

	BOOST_FOREACH (const String& line, lines) {
		size_t delim = line.FindFirstOf("|");

		if (!text.IsEmpty())
			text += "\n";

		if (delim != String::NPos) {
			text += line.SubStr(0, delim);

			if (!perfdata.IsEmpty())
				perfdata += " ";

			perfdata += line.SubStr(delim + 1, line.GetLength());
		} else {
			text += line;
		}
	}

	result->Set("output", text);
	result->Set("performance_data_raw", perfdata);

	return result;
}
