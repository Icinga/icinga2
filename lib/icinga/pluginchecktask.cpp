/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
#include "icinga/macroprocessor.h"
#include "base/dynamictype.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(PluginCheck,  &PluginCheckTask::ScriptFunc);

PluginCheckTask::PluginCheckTask(const ScriptTask::Ptr& task, const Process::Ptr& process, const Value& command)
	: m_Task(task), m_Process(process), m_Command(command)
{ }

/**
 * @threadsafety Always.
 */
void PluginCheckTask::ScriptFunc(const ScriptTask::Ptr& task, const std::vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: Service must be specified."));

	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: Macros must be specified."));

	Service::Ptr service = arguments[0];
	Dictionary::Ptr macros = arguments[1];

	Value raw_command = service->GetCheckCommand();
	Value command = MacroProcessor::ResolveMacros(raw_command, macros);

	Process::Ptr process = boost::make_shared<Process>(Process::SplitCommand(command), macros);

	PluginCheckTask ct(task, process, command);

	process->Start(boost::bind(&PluginCheckTask::ProcessFinishedHandler, ct));
}

/**
 * @threadsafety Always.
 */
void PluginCheckTask::ProcessFinishedHandler(PluginCheckTask ct)
{
	ProcessResult pr;

	try {
		pr = ct.m_Process->GetResult();
	} catch (...) {
		ct.m_Task->FinishException(boost::current_exception());

		return;
	}

	String output = pr.Output;
	output.Trim();
	Dictionary::Ptr result = ParseCheckOutput(output);
	result->Set("command", ct.m_Command);
	result->Set("state", ExitStatusToState(pr.ExitStatus));
	result->Set("execution_start", pr.ExecutionStart);
	result->Set("execution_end", pr.ExecutionEnd);

	ct.m_Task->FinishResult(result);
}

/**
 * @threadsafety Always.
 */
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

/**
 * @threadsafety Always.
 */
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
