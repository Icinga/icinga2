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

#include "icinga/pluginutility.hpp"
#include "icinga/macroprocessor.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/process.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace icinga;

void PluginUtility::ExecuteCommand(const Command::Ptr& commandObj, const Checkable::Ptr& checkable,
    const CheckResult::Ptr& cr, const MacroProcessor::ResolverList& macroResolvers,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros,
    const std::function<void(const Value& commandLine, const ProcessResult&)>& callback)
{
	Value raw_command = commandObj->GetCommandLine();
	Dictionary::Ptr raw_arguments = commandObj->GetArguments();

	Value command;

	try {
		command = MacroProcessor::ResolveArguments(raw_command, raw_arguments,
		    macroResolvers, cr, resolvedMacros, useResolvedMacros);
	} catch (const std::exception& ex) {
		String message = DiagnosticInformation(ex);

		Log(LogWarning, "PluginUtility", message);

		if (callback) {
			ProcessResult pr;
			pr.PID = -1;
			pr.ExecutionStart = Utility::GetTime();
			pr.ExecutionEnd = pr.ExecutionStart;
			pr.ExitStatus = 3; /* Unknown */
			pr.Output = message;
			callback(Empty, pr);
		}

		return;
	}

	Dictionary::Ptr envMacros = new Dictionary();

	Dictionary::Ptr env = commandObj->GetEnv();

	if (env) {
		ObjectLock olock(env);
		for (const Dictionary::Pair& kv : env) {
			String name = kv.second;

			Value value = MacroProcessor::ResolveMacros(name, macroResolvers, cr,
			    NULL, MacroProcessor::EscapeCallback(), resolvedMacros,
			    useResolvedMacros);

			if (value.IsObjectType<Array>())
				value = Utility::Join(value, ';');

			envMacros->Set(kv.first, value);
		}
	}

	if (resolvedMacros && !useResolvedMacros)
		return;

	Process::Ptr process = new Process(Process::PrepareCommand(command), envMacros);

	if (checkable->GetCheckTimeout().IsEmpty())
		process->SetTimeout(commandObj->GetTimeout());
	else
		process->SetTimeout(checkable->GetCheckTimeout());

	process->SetAdjustPriority(true);

	process->Run(std::bind(callback, command, _1));
}

ServiceState PluginUtility::ExitStatusToState(int exitStatus)
{
	switch (exitStatus) {
		case 0:
			return ServiceOK;
		case 1:
			return ServiceWarning;
		case 2:
			return ServiceCritical;
		default:
			return ServiceUnknown;
	}
}

std::pair<String, String> PluginUtility::ParseCheckOutput(const String& output)
{
	String text;
	String perfdata;

	std::vector<String> lines;
	boost::algorithm::split(lines, output, boost::is_any_of("\r\n"));

	for (const String& line : lines) {
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

	boost::algorithm::trim(perfdata);

	return std::make_pair(text, perfdata);
}

Array::Ptr PluginUtility::SplitPerfdata(const String& perfdata)
{
	Array::Ptr result = new Array();

	size_t begin = 0;
	String multi_prefix;

	for (;;) {
		size_t eqp = perfdata.FindFirstOf('=', begin);

		if (eqp == String::NPos)
			break;

		String label = perfdata.SubStr(begin, eqp - begin);

		if (label.GetLength() > 2 && label[0] == '\'' && label[label.GetLength() - 1] == '\'')
			label = label.SubStr(1, label.GetLength() - 2);

		size_t multi_index = label.RFind("::");

		if (multi_index != String::NPos)
			multi_prefix = "";

		size_t spq = perfdata.FindFirstOf(' ', eqp);

		if (spq == String::NPos)
			spq = perfdata.GetLength();

		String value = perfdata.SubStr(eqp + 1, spq - eqp - 1);

		if (!multi_prefix.IsEmpty())
			label = multi_prefix + "::" + label;

		String pdv;
		if (label.FindFirstOf(" ") != String::NPos)
			pdv = "'" + label + "'=" + value;
		else
			pdv = label + "=" + value;

		result->Add(pdv);

		if (multi_index != String::NPos)
			multi_prefix = label.SubStr(0, multi_index);

		begin = spq + 1;
	}

	return result;
}

String PluginUtility::FormatPerfdata(const Array::Ptr& perfdata)
{
	if (!perfdata)
		return "";

	std::ostringstream result;

	ObjectLock olock(perfdata);

	bool first = true;
	for (const Value& pdv : perfdata) {
		if (!first)
			result << " ";
		else
			first = false;

		if (pdv.IsObjectType<PerfdataValue>())
			result << static_cast<PerfdataValue::Ptr>(pdv)->Format();
		else
			result << pdv;
	}

	return result.str();
}
