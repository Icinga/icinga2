/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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
#include <boost/algorithm/string/trim.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

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

std::pair<String, Value> PluginUtility::ParseCheckOutput(const String& output)
{
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

	boost::algorithm::trim(perfdata);

	return std::make_pair(text, ParsePerfdata(perfdata));
}

Value PluginUtility::ParsePerfdata(const String& perfdata)
{
	try {
		Dictionary::Ptr result = make_shared<Dictionary>();
	
		size_t begin = 0;
		String multi_prefix;
		
		for (;;) {
			size_t eqp = perfdata.FindFirstOf('=', begin);

			if (eqp == String::NPos)
				break;

			String key = perfdata.SubStr(begin, eqp - begin);

			if (key.GetLength() > 2 && key[0] == '\'' && key[key.GetLength() - 1] == '\'')
				key = key.SubStr(1, key.GetLength() - 2);

			size_t multi_index = key.RFind("::");

			if (multi_index != String::NPos)
				multi_prefix = "";

			size_t spq = perfdata.FindFirstOf(' ', eqp);

			if (spq == String::NPos)
				spq = perfdata.GetLength();

			String value = perfdata.SubStr(eqp + 1, spq - eqp - 1);

			if (!multi_prefix.IsEmpty())
				key = multi_prefix + "::" + key;

			result->Set(key, PerfdataValue::Parse(value));

			if (multi_index != String::NPos)
				multi_prefix = key.SubStr(0, multi_index);

			begin = spq + 1;
		}

		return result;
	} catch (const std::exception&) {
		return perfdata;
	}
}

String PluginUtility::FormatPerfdata(const Value& perfdata)
{
	std::ostringstream result;

	if (!perfdata.IsObjectType<Dictionary>())
		return perfdata;

	Dictionary::Ptr dict = perfdata;

	ObjectLock olock(dict);

	bool first = true;
	BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
		String key;
		if (kv.first.FindFirstOf(" ") != String::NPos)
			key = "'" + kv.first + "'";
		else
			key = kv.first;

		if (!first)
			result << " ";
		else
			first = false;

		result << key << "=" << PerfdataValue::Format(kv.second);
	}

	return result.str();
}
