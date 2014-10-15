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

#include "cli/objectlistcommand.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/dynamicobject.hpp"
#include "base/dynamictype.hpp"
#include "base/serializer.hpp"
#include "base/netstring.hpp"
#include "base/stdiostream.hpp"
#include "base/debug.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>

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
    boost::program_options::options_description& hiddenDesc,
    ArgumentCompletionDescription& argCompletionDesc) const
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
	if (!ap.empty()) {
		Log(LogWarning, "cli", "Ignoring parameters: " + boost::algorithm::join(ap, " "));
	}

	String objectfile = Application::GetObjectsPath();

	if (!Utility::PathExists(objectfile)) {
		Log(LogCritical, "cli", "Cannot parse objects file '" + Application::GetObjectsPath() + "'.");
		Log(LogCritical, "cli", "Run 'icinga2 daemon -C' to validate config and generate the cache file.");
		return 1;
	}

	std::fstream fp;
	fp.open(objectfile.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = make_shared<StdioStream>(&fp, false);
	unsigned long objects_count = 0;
	std::map<String, int> type_count;

	String message;
	String name_filter, type_filter;

	if (vm.count("name"))
		name_filter = vm["name"].as<std::string>();
	if (vm.count("type"))
		type_filter = vm["type"].as<std::string>();

	while (NetString::ReadStringFromStream(sfp, &message)) {
		ReadObject(message, type_count, name_filter, type_filter);
		objects_count++;
	}

	sfp->Close();
	fp.close();

	if (vm.count("count"))
		std::cout << FormatTypeCounts(type_count) << std::endl;

	Log(LogNotice, "cli", "Parsed " + Convert::ToString(objects_count) + " objects.");

	return 0;
}

void ObjectListCommand::ReadObject(const String& message, std::map<String, int>& type_count, const String& name_filter, const String& type_filter)
{
	Dictionary::Ptr object = JsonDeserialize(message);

	Dictionary::Ptr properties = object->Get("properties");

	String internal_name = properties->Get("__name");
	String name = object->Get("name");
	String type = object->Get("type");

	if (!name_filter.IsEmpty() && !Utility::Match(name_filter, name) && !Utility::Match(name_filter, internal_name))
		return;
	if (!type_filter.IsEmpty() && !Utility::Match(type_filter, type))
		return;

	bool abstract = object->Get("abstract");
	Dictionary::Ptr debug_hints = object->Get("debug_hints");

	std::ostringstream msgbuf;

	if (abstract)
		msgbuf << "Template '";
	else
		msgbuf << "Object '";

	msgbuf << "\x1b[1;34m" << internal_name << "\x1b[0m" << "'"; //blue
	msgbuf << " of type '" << "\x1b[1;34m" << type << "\x1b[0m" << "':\n"; //blue

	msgbuf << FormatProperties(properties, debug_hints, 2);

	std::cout << msgbuf.str() << "\n";

	type_count[type]++;
}

String ObjectListCommand::FormatProperties(const Dictionary::Ptr& props, const Dictionary::Ptr& debug_hints, int indent)
{
	std::ostringstream msgbuf;

	/* get debug hint props */
	Dictionary::Ptr debug_hint_props;
	if (debug_hints)
		debug_hint_props = debug_hints->Get("properties");

	int offset = 2;

	BOOST_FOREACH(const Dictionary::Pair& kv, props) {
		String key = kv.first;
		Value val = kv.second;

		/* key & value */
		msgbuf << std::setw(indent) << " " << "* " << "\x1b[1;32m" << key << "\x1b[0m"; //green

		/* extract debug hints for key */
		Dictionary::Ptr debug_hints_fwd;
		if (debug_hint_props)
			debug_hints_fwd = debug_hint_props->Get(key);

		/* print dicts recursively */
		if (val.IsObjectType<Dictionary>()) {
			msgbuf << "\n";
			msgbuf << FormatHints(debug_hints_fwd, indent + offset);
			msgbuf << FormatProperties(val, debug_hints_fwd, indent + offset);
		} else {
			msgbuf << " = " << FormatValue(val) << "\n";
			msgbuf << FormatHints(debug_hints_fwd, indent + offset);
		}
	}

	return msgbuf.str();
}

String ObjectListCommand::FormatHints(const Dictionary::Ptr& debug_hints, int indent)
{
	if (!debug_hints)
		return String();

	Array::Ptr messages = debug_hints->Get("messages");
	String hints;

	BOOST_FOREACH(const Value& msg, messages) {
		hints += FormatHint(msg, indent);
	}

	return hints;
}

String ObjectListCommand::FormatHint(const Array::Ptr& msg, int indent)
{
	std::ostringstream msgbuf;
	msgbuf << std::setw(indent) << " " << "\x1b[1;36m" "% " << msg->Get(0) << " modified in '" << msg->Get(1) << "', lines "
	    << msg->Get(2) << ":" << msg->Get(3) << "-" << msg->Get(4) << ":" << msg->Get(5) << "\x1b[0m" "\n"; //cyan

	return msgbuf.str();
}

String ObjectListCommand::FormatTypeCounts(const std::map<String, int>& type_count)
{
	std::ostringstream msgbuf;

	typedef std::map<String, int>::value_type TypeCount;

	BOOST_FOREACH(const TypeCount& kv, type_count) {
		msgbuf << "Found " << kv.second << " " << kv.first << " objects.\n";
	}

	return msgbuf.str();
}

String ObjectListCommand::FormatValue(const Value& val)
{
	if (val.IsObjectType<Array>())
		return "[ " + FormatArray(val) + " ]";

	if (val.IsString())
		return "'" + Convert::ToString(val) + "'";

	return Convert::ToString(val);
}


String ObjectListCommand::FormatArray(const Array::Ptr& arr)
{
	bool first = true;
	String str;

	if (arr) {
		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& value, arr) {
			if (first)
				first = false;
			else
				str += ", ";

			str += FormatValue(value);
		}
	}

	return str;
}
