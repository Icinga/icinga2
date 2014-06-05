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

#include "base/scriptutils.hpp"
#include "base/scriptfunction.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/serializer.hpp"
#include "base/logger_fwd.hpp"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <set>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(regex, &ScriptUtils::Regex);
REGISTER_SCRIPTFUNCTION(match, &Utility::Match);
REGISTER_SCRIPTFUNCTION(len, &ScriptUtils::Len);
REGISTER_SCRIPTFUNCTION(union, &ScriptUtils::Union);
REGISTER_SCRIPTFUNCTION(intersection, &ScriptUtils::Intersection);
REGISTER_SCRIPTFUNCTION(log, &ScriptUtils::Log);
REGISTER_SCRIPTFUNCTION(range, &ScriptUtils::Range);
REGISTER_SCRIPTFUNCTION(exit, &ScriptUtils::Exit);

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

int ScriptUtils::Len(const Value& value)
{
	if (value.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = value;
		return dict->GetLength();
	} else if (value.IsObjectType<Array>()) {
		Array::Ptr array = value;
		return array->GetLength();
	} else {
		return Convert::ToString(value).GetLength();
	}
}

Array::Ptr ScriptUtils::Union(const std::vector<Value>& arguments)
{
	std::set<Value> values;

	BOOST_FOREACH(const Value& varr, arguments) {
		Array::Ptr arr = varr;

		BOOST_FOREACH(const Value& value, arr) {
			values.insert(value);
		}
	}

	Array::Ptr result = make_shared<Array>();
	BOOST_FOREACH(const Value& value, values) {
		result->Add(value);
	}

	return result;
}

Array::Ptr ScriptUtils::Intersection(const std::vector<Value>& arguments)
{
	if (arguments.size() == 0)
		return make_shared<Array>();

	Array::Ptr result = make_shared<Array>();

	Array::Ptr arr1 = static_cast<Array::Ptr>(arguments[0])->ShallowClone();

	for (std::vector<Value>::size_type i = 1; i < arguments.size(); i++) {
		std::sort(arr1->Begin(), arr1->End());

		Array::Ptr arr2 = static_cast<Array::Ptr>(arguments[i])->ShallowClone();
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

	if (message.IsString())
		::Log(severity, facility, message);
	else
		::Log(severity, facility, JsonSerialize(message));
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

	Array::Ptr result = make_shared<Array>();

	if ((start < end && increment <= 0) ||
	    (start > end && increment >= 0))
		return result;

	for (double i = start; i < end; i += increment) {
		result->Add(i);
	}

	return result;
}

void ScriptUtils::Exit(int code)
{
	exit(code);
}
