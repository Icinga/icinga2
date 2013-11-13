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

#include "icinga/pluginutility.h"
#include "icinga/checkcommand.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "icinga/perfdatavalue.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/scriptfunction.h"
#include "base/utility.h"
#include "base/process.h"
#include "base/objectlock.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

ServiceState PluginUtility::ExitStatusToState(int exitStatus)
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

CheckResult::Ptr PluginUtility::ParseCheckOutput(const String& output)
{
	CheckResult::Ptr result = make_shared<CheckResult>();

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

	result->SetOutput(text);
	result->SetPerformanceData(ParsePerfdata(perfdata));

	return result;
}

Value PluginUtility::ParsePerfdata(const String& perfdata)
{
	try {
		Dictionary::Ptr result = make_shared<Dictionary>();
	
		size_t begin = 0;
		
		for (;;) {
			size_t eqp = perfdata.FindFirstOf('=', begin);

			if (eqp == String::NPos)
				break;

			String key = perfdata.SubStr(begin, eqp - begin);

			size_t spq = perfdata.FindFirstOf(' ', eqp);

			if (spq == String::NPos)
				spq = perfdata.GetLength();

			String value = perfdata.SubStr(eqp + 1, spq - eqp - 1);

			result->Set(key, PerfdataValue::Parse(value));

			begin = spq + 1;
		}

		return result;
	} catch (const std::exception&) {
		return perfdata;
	}
}

String PluginUtility::FormatPerfdata(const Value& perfdata)
{
	String output;

	if (!perfdata.IsObjectType<Dictionary>())
		return perfdata;

	Dictionary::Ptr dict = perfdata;

	ObjectLock olock(dict);

	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), dict) {
		if (!output.IsEmpty())
			output += " ";

		output += key + "=" + PerfdataValue::Format(value);
	}

	return output;
}
