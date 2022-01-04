/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/objectlistutility.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"
#include "base/console.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <iostream>
#include <iomanip>

using namespace icinga;

bool ObjectListUtility::PrintObject(std::ostream& fp, bool& first, const String& message, std::map<String, int>& type_count, const String& name_filter, const String& type_filter)
{
	Dictionary::Ptr object = JsonDecode(message);

	Dictionary::Ptr properties = object->Get("properties");

	String internal_name = properties->Get("__name");
	String name = object->Get("name");
	String type = object->Get("type");

	if (!name_filter.IsEmpty() && !Utility::Match(name_filter, name) && !Utility::Match(name_filter, internal_name))
		return false;
	if (!type_filter.IsEmpty() && !Utility::Match(type_filter, type))
		return false;

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
	return true;
}

void ObjectListUtility::PrintProperties(std::ostream& fp, const Dictionary::Ptr& props, const Dictionary::Ptr& debug_hints, int indent)
{
	/* get debug hint props */
	Dictionary::Ptr debug_hint_props;
	if (debug_hints)
		debug_hint_props = debug_hints->Get("properties");

	int offset = 2;

	ObjectLock olock(props);
	for (const auto& kv : props)
	{
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

void ObjectListUtility::PrintHints(std::ostream& fp, const Dictionary::Ptr& debug_hints, int indent)
{
	if (!debug_hints)
		return;

	Array::Ptr messages = debug_hints->Get("messages");

	if (messages) {
		ObjectLock olock(messages);

		for (const auto& msg : messages)
		{
			PrintHint(fp, msg, indent);
		}
	}
}

void ObjectListUtility::PrintHint(std::ostream& fp, const Array::Ptr& msg, int indent)
{
	fp << std::setw(indent) << " " << ConsoleColorTag(Console_ForegroundCyan) << "% " << msg->Get(0) << " modified in '" << msg->Get(1) << "', lines "
		<< msg->Get(2) << ":" << msg->Get(3) << "-" << msg->Get(4) << ":" << msg->Get(5) << ConsoleColorTag(Console_Normal) << "\n";
}

void ObjectListUtility::PrintValue(std::ostream& fp, const Value& val)
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

void ObjectListUtility::PrintArray(std::ostream& fp, const Array::Ptr& arr)
{
	bool first = true;

	fp << "[ ";

	if (arr) {
		ObjectLock olock(arr);
		for (const auto& value : arr)
		{
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
