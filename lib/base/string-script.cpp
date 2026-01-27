// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	std::vector<String> tokens = self.Split(delims.CStr());

	return Array::FromVector(tokens);
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
	static Dictionary::Ptr prototype = new Dictionary({
		{ "len", new Function("String#len", StringLen, {}, true) },
		{ "to_string", new Function("String#to_string", StringToString, {}, true) },
		{ "substr", new Function("String#substr", StringSubstr, { "start", "len" }, true) },
		{ "upper", new Function("String#upper", StringUpper, {}, true) },
		{ "lower", new Function("String#lower", StringLower, {}, true) },
		{ "split", new Function("String#split", StringSplit, { "delims" }, true) },
		{ "find", new Function("String#find", StringFind, { "str", "start" }, true) },
		{ "contains", new Function("String#contains", StringContains, { "str" }, true) },
		{ "replace", new Function("String#replace", StringReplace, { "search", "replacement" }, true) },
		{ "reverse", new Function("String#reverse", StringReverse, {}, true) },
		{ "trim", new Function("String#trim", StringTrim, {}, true) }
	});

	return prototype;
}
