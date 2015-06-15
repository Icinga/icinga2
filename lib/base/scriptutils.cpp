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

#include "base/scriptutils.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/dynamictype.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <set>
#ifdef _WIN32
#include <msi.h>
#endif /* _WIN32 */

using namespace icinga;

REGISTER_SCRIPTFUNCTION(regex, &ScriptUtils::Regex);
REGISTER_SCRIPTFUNCTION(match, &Utility::Match);
REGISTER_SCRIPTFUNCTION(len, &ScriptUtils::Len);
REGISTER_SCRIPTFUNCTION(union, &ScriptUtils::Union);
REGISTER_SCRIPTFUNCTION(intersection, &ScriptUtils::Intersection);
REGISTER_SCRIPTFUNCTION(log, &ScriptUtils::Log);
REGISTER_SCRIPTFUNCTION(range, &ScriptUtils::Range);
REGISTER_SCRIPTFUNCTION(exit, &Application::Exit);
REGISTER_SCRIPTFUNCTION(typeof, &ScriptUtils::TypeOf);
REGISTER_SCRIPTFUNCTION(keys, &ScriptUtils::Keys);
REGISTER_SCRIPTFUNCTION(random, &Utility::Random);
REGISTER_SCRIPTFUNCTION(get_object, &ScriptUtils::GetObject);
REGISTER_SCRIPTFUNCTION(get_objects, &ScriptUtils::GetObjects);
REGISTER_SCRIPTFUNCTION(assert, &ScriptUtils::Assert);
REGISTER_SCRIPTFUNCTION(string, &ScriptUtils::CastString);
REGISTER_SCRIPTFUNCTION(number, &ScriptUtils::CastNumber);
REGISTER_SCRIPTFUNCTION(bool, &ScriptUtils::CastBool);
REGISTER_SCRIPTFUNCTION(get_time, &Utility::GetTime);
REGISTER_SCRIPTFUNCTION(basename, &Utility::BaseName);
REGISTER_SCRIPTFUNCTION(dirname, &Utility::DirName);
REGISTER_SCRIPTFUNCTION(msi_get_component_path, &ScriptUtils::MsiGetComponentPathShim);

String ScriptUtils::CastString(const Value& value)
{
	return value;
}

double ScriptUtils::CastNumber(const Value& value)
{
	return value;
}

bool ScriptUtils::CastBool(const Value& value)
{
	return value.ToBool();
}
bool ScriptUtils::Regex(const String& pattern, const String& text)
{
	bool res = false;
	try {
		boost::regex expr(pattern.GetData());
		boost::smatch what;
		res = boost::regex_search(text.GetData(), what, expr);
	} catch (boost::exception&) {
		res = false; /* exception means something went terribly wrong */
	}

	return res;
}

double ScriptUtils::Len(const Value& value)
{
	if (value.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = value;
		return dict->GetLength();
	} else if (value.IsObjectType<Array>()) {
		Array::Ptr array = value;
		return array->GetLength();
	} else if (value.IsString()) {
		return Convert::ToString(value).GetLength();
	} else {
		return 0;
	}
}

Array::Ptr ScriptUtils::Union(const std::vector<Value>& arguments)
{
	std::set<Value> values;

	BOOST_FOREACH(const Value& varr, arguments) {
		Array::Ptr arr = varr;

		if (arr) {
			BOOST_FOREACH(const Value& value, arr) {
				values.insert(value);
			}
		}
	}

	Array::Ptr result = new Array();
	BOOST_FOREACH(const Value& value, values) {
		result->Add(value);
	}

	return result;
}

Array::Ptr ScriptUtils::Intersection(const std::vector<Value>& arguments)
{
	if (arguments.size() == 0)
		return new Array();

	Array::Ptr result = new Array();

	Array::Ptr arg1 = arguments[0];

	if (!arg1)
		return result;

	Array::Ptr arr1 = arg1->ShallowClone();

	for (std::vector<Value>::size_type i = 1; i < arguments.size(); i++) {
		std::sort(arr1->Begin(), arr1->End());

		Array::Ptr arg2 = arguments[i];

		if (!arg2)
			return result;

		Array::Ptr arr2 = arg2->ShallowClone();
		std::sort(arr2->Begin(), arr2->End());

		result->Resize(std::max(arr1->GetLength(), arr2->GetLength()));
		Array::Iterator it = std::set_intersection(arr1->Begin(), arr1->End(), arr2->Begin(), arr2->End(), result->Begin());
		result->Resize(it - result->Begin());
		arr1 = result;
	}

	return result;
}

void ScriptUtils::Log(const std::vector<Value>& arguments)
{
	if (arguments.size() != 1 && arguments.size() != 3)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid number of arguments for log()"));

	LogSeverity severity;
	String facility;
	Value message;

	if (arguments.size() == 1) {
		severity = LogInformation;
		facility = "config";
		message = arguments[0];
	} else {
		int sval = static_cast<int>(arguments[0]);
		severity = static_cast<LogSeverity>(sval);
		facility = arguments[1];
		message = arguments[2];
	}

	if (message.IsString() || (!message.IsObjectType<Array>() && !message.IsObjectType<Dictionary>()))
		::Log(severity, facility, message);
	else
		::Log(severity, facility, JsonEncode(message));
}

Array::Ptr ScriptUtils::Range(const std::vector<Value>& arguments)
{
	double start, end, increment;

	switch (arguments.size()) {
		case 1:
			start = 0;
			end = arguments[0];
			increment = 1;
			break;
		case 2:
			start = arguments[0];
			end = arguments[1];
			increment = 1;
			break;
		case 3:
			start = arguments[0];
			end = arguments[1];
			increment = arguments[2];
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid number of arguments for range()"));
	}

	Array::Ptr result = new Array();

	if ((start < end && increment <= 0) ||
	    (start > end && increment >= 0))
		return result;

	for (double i = start; i < end; i += increment) {
		result->Add(i);
	}

	return result;
}

Type::Ptr ScriptUtils::TypeOf(const Value& value)
{
	switch (value.GetType()) {
		case ValueEmpty:
			return Type::GetByName("Object");
		case ValueNumber:
			return Type::GetByName("Number");
		case ValueBoolean:
			return Type::GetByName("Boolean");
		case ValueString:
			return Type::GetByName("String");
		case ValueObject:
			return static_cast<Object::Ptr>(value)->GetReflectionType();
		default:
			VERIFY(!"Invalid value type.");
	}
}

Array::Ptr ScriptUtils::Keys(const Dictionary::Ptr& dict)
{
	Array::Ptr result = new Array();

	if (dict) {
		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			result->Add(kv.first);
		}
	}

	return result;
}

DynamicObject::Ptr ScriptUtils::GetObject(const Type::Ptr& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type->GetName());

	if (!dtype)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type name"));

	return dtype->GetObject(name);
}

Array::Ptr ScriptUtils::GetObjects(const Type::Ptr& type)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type->GetName());

	if (!dtype)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid type name"));

	Array::Ptr result = new Array();

	BOOST_FOREACH(const DynamicObject::Ptr& object, dtype->GetObjects())
		result->Add(object);

	return result;
}

void ScriptUtils::Assert(const Value& arg)
{
	if (!arg.ToBool())
		BOOST_THROW_EXCEPTION(std::runtime_error("Assertion failed"));
}

String ScriptUtils::MsiGetComponentPathShim(const String& component)
{
#ifdef _WIN32
	TCHAR productCode[39];
	if (MsiGetProductCode(component.CStr(), productCode) != ERROR_SUCCESS)
		return "";
	TCHAR path[2048];
	DWORD szPath = sizeof(path);
	path[0] = '\0';
	MsiGetComponentPath(productCode, component.CStr(), path, &szPath);
	return path;
#else /* _WIN32 */
	return String();
#endif /* _WIN32 */
}
