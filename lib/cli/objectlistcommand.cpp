/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/objectlistcommand.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/dynamicobject.hpp"
#include "base/dynamictype.hpp"
#include "base/json.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("object/list", ObjectListCommand);

String ObjectListCommand::GetDescription(void) const
{
	return "Lists all Icinga 2 objects.";
}

String ObjectListCommand::GetShortDescription(void) const
{
	return "lists all objects";
}

void ObjectListCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("count,c", "display object counts by types")
		("name,n", po::value<std::string>(), "filter by name matches")
		("type,t", po::value<std::string>(), "filter by type matches");
}

/**
 * The entry point for the "object list" CLI command.
 *
 * @returns An exit status.
 */
int ObjectListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String objectfile = Application::GetObjectsPath();

	if (!Utility::PathExists(objectfile)) {
		Log(LogCritical, "cli")
		    << "Cannot open objects file '" << Application::GetObjectsPath() << "'.";
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	std::fstream fp;
	fp.open(objectfile.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = new StdioStream(&fp, false);
	unsigned long objects_count = 0;
	std::map<String, int> type_count;

	String message;
	String name_filter, type_filter;

	if (vm.count("name"))
		name_filter = vm["name"].as<std::string>();
	if (vm.count("type"))
		type_filter = vm["type"].as<std::string>();

	bool first = true;

	while (NetString::ReadStringFromStream(sfp, &message)) {
		PrintObject(std::cout, first, message, type_count, name_filter, type_filter);
		objects_count++;
	}

	sfp->Close();
	fp.close();

	if (vm.count("count")) {
		if (!first)
			std::cout << "\n";

		PrintTypeCounts(std::cout, type_count);
		std::cout << "\n";
	}

	Log(LogNotice, "cli")
	    << "Parsed " << objects_count << " objects.";

	return 0;
}

void ObjectListCommand::PrintObject(std::ostream& fp, bool& first, const String& message, std::map<String, int>& type_count, const String& name_filter, const String& type_filter)
{
	Dictionary::Ptr object = JsonDecode(message);

	Dictionary::Ptr properties = object->Get("properties");

	String internal_name = properties->Get("__name");
	String name = object->Get("name");
	String type = object->Get("type");

	if (!name_filter.IsEmpty() && !Utility::Match(name_filter, name) && !Utility::Match(name_filter, internal_name))
		return;
	if (!type_filter.IsEmpty() && !Utility::Match(type_filter, type))
		return;

	if (first)
		first = false;
	else
		fp << "\n";

	Dictionary::Ptr debug_hints = object->Get("debug_hints");

	fp << "Object '" << ConsoleColorTag(Console_ForegroundBlue | Console_Bold) << internal_name << ConsoleColorTag(Console_Normal) << "'";
	fp << " of type '" << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << type << ConsoleColorTag(Console_Normal) << "':\n";

	Array::Ptr di = object->Get("debug_info");

	if (di) {
		fp << ConsoleColorTag(Console_ForegroundCyan) << "  % declared in '" << di->Get(0) << "', lines "
		    << di->Get(1) << ":" << di->Get(2) << "-" << di->Get(3) << ":" << di->Get(4) << ConsoleColorTag(Console_Normal) << "\n";
	}

	PrintProperties(fp, properties, debug_hints, 2);

	type_count[type]++;
}

void ObjectListCommand::PrintProperties(std::ostream& fp, const Dictionary::Ptr& props, const Dictionary::Ptr& debug_hints, int indent)
{
	/* get debug hint props */
	Dictionary::Ptr debug_hint_props;
	if (debug_hints)
		debug_hint_props = debug_hints->Get("properties");

	int offset = 2;

	ObjectLock olock(props);
	BOOST_FOREACH(const Dictionary::Pair& kv, props) {
		String key = kv.first;
		Value val = kv.second;

		/* key & value */
		fp << std::setw(indent) << " " << "* " << ConsoleColorTag(Console_ForegroundGreen) << key << ConsoleColorTag(Console_Normal);

		/* extract debug hints for key */
		Dictionary::Ptr debug_hints_fwd;
		if (debug_hint_props)
			debug_hints_fwd = debug_hint_props->Get(key);

		/* print dicts recursively */
		if (val.IsObjectType<Dictionary>()) {
			fp << "\n";
			PrintHints(fp, debug_hints_fwd, indent + offset);
			PrintProperties(fp, val, debug_hints_fwd, indent + offset);
		} else {
			fp << " = ";
			PrintValue(fp, val);
			fp << "\n";
			PrintHints(fp, debug_hints_fwd, indent + offset);
		}
	}
}

void ObjectListCommand::PrintHints(std::ostream& fp, const Dictionary::Ptr& debug_hints, int indent)
{
	if (!debug_hints)
		return;

	Array::Ptr messages = debug_hints->Get("messages");

	if (messages) {
		ObjectLock olock(messages);

		BOOST_FOREACH(const Value& msg, messages) {
			PrintHint(fp, msg, indent);
		}
	}
}

void ObjectListCommand::PrintHint(std::ostream& fp, const Array::Ptr& msg, int indent)
{
	fp << std::setw(indent) << " " << ConsoleColorTag(Console_ForegroundCyan) << "% " << msg->Get(0) << " modified in '" << msg->Get(1) << "', lines "
	    << msg->Get(2) << ":" << msg->Get(3) << "-" << msg->Get(4) << ":" << msg->Get(5) << ConsoleColorTag(Console_Normal) << "\n";
}

void ObjectListCommand::PrintTypeCounts(std::ostream& fp, const std::map<String, int>& type_count)
{
	typedef std::map<String, int>::value_type TypeCount;

	BOOST_FOREACH(const TypeCount& kv, type_count) {
		fp << "Found " << kv.second << " " << kv.first << " object";

		if (kv.second != 1)
			fp << "s";

		fp << ".\n";
	}
}

void ObjectListCommand::PrintValue(std::ostream& fp, const Value& val)
{
	if (val.IsObjectType<Array>()) {
		PrintArray(fp, val);
		return;
	}

	if (val.IsString()) {
		fp << "\"" << Convert::ToString(val) << "\"";
		return;
	}

	if (val.IsEmpty()) {
		fp << "null";
		return;
	}

	fp << Convert::ToString(val);
}

void ObjectListCommand::PrintArray(std::ostream& fp, const Array::Ptr& arr)
{
	bool first = true;

	fp << "[ ";

	if (arr) {
		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& value, arr) {
			if (first)
				first = false;
			else
				fp << ", ";

			PrintValue(fp, value);
		}
	}

	if (!first)
		fp << " ";

	fp << "]";
}
