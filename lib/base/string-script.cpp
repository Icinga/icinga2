/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/object.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string.hpp>

using namespace icinga;

static int StringLen()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return self.GetLength();
}

static String StringToString()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	return vframe->Self;
}

static String StringSubstr(const std::vector<Value>& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;

	if (args.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments"));

	if (static_cast<double>(args[0]) < 0 || static_cast<double>(args[0]) >= self.GetLength())
		BOOST_THROW_EXCEPTION(std::invalid_argument("String index is out of range"));

	if (args.size() > 1)
		return self.SubStr(args[0], args[1]);
	else
		return self.SubStr(args[0]);
}

static String StringUpper()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return boost::to_upper_copy(self);
}

static String StringLower()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return boost::to_lower_copy(self);
}

static Array::Ptr StringSplit(const String& delims)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	std::vector<String> tokens;
	boost::algorithm::split(tokens, self, boost::is_any_of(delims));

	Array::Ptr result = new Array();
	for (const String& token : tokens) {
		result->Add(token);
	}
	return result;
}

static int StringFind(const std::vector<Value>& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;

	if (args.empty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments"));

	String::SizeType result;

	if (args.size() > 1) {
		if (static_cast<double>(args[1]) < 0)
			BOOST_THROW_EXCEPTION(std::invalid_argument("String index is out of range"));

		result = self.Find(args[0], args[1]);
	} else
		result = self.Find(args[0]);

	if (result == String::NPos)
		return -1;
	else
		return result;
}

static bool StringContains(const String& str)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return self.Contains(str);
}

static Value StringReplace(const String& search, const String& replacement)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;

	boost::algorithm::replace_all(self, search, replacement);
	return self;
}

static String StringReverse()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return self.Reverse();
}

static String StringTrim()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return self.Trim();
}

Object::Ptr String::GetPrototype()
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new Function("String#len", StringLen, {}, true));
		prototype->Set("to_string", new Function("String#to_string", StringToString, {}, true));
		prototype->Set("substr", new Function("String#substr", StringSubstr, { "start", "len" }, true));
		prototype->Set("upper", new Function("String#upper", StringUpper, {}, true));
		prototype->Set("lower", new Function("String#lower", StringLower, {}, true));
		prototype->Set("split", new Function("String#split", StringSplit, { "delims" }, true));
		prototype->Set("find", new Function("String#find", StringFind, { "str", "start" }, true));
		prototype->Set("contains", new Function("String#contains", StringContains, { "str" }, true));
		prototype->Set("replace", new Function("String#replace", StringReplace, { "search", "replacement" }, true));
		prototype->Set("reverse", new Function("String#reverse", StringReverse, {}, true));
		prototype->Set("trim", new Function("String#trim", StringTrim, {}, true));
	}

	return prototype;
}

