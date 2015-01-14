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

#include "base/object.hpp"
#include "base/dictionary.hpp"
#include "base/scriptfunction.hpp"
#include "base/scriptfunctionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static int StringLen(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return self.GetLength();
}

static String StringToString(void)
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

static String StringUpper(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;
	return boost::to_upper_copy(self);
}

static String StringLower(void)
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
	BOOST_FOREACH(const String& token, tokens) {
		result->Add(token);
	}
	return result;
}

static Value StringFind(const std::vector<Value>& args)
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

static Value StringReplace(const String& search, const String& replacement)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	String self = vframe->Self;

	boost::algorithm::replace_all(self, search, replacement);
	return self;
}


Object::Ptr String::GetPrototype(void)
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new ScriptFunction(WrapScriptFunction(StringLen)));
		prototype->Set("to_string", new ScriptFunction(WrapScriptFunction(StringToString)));
		prototype->Set("substr", new ScriptFunction(WrapScriptFunction(StringSubstr)));
		prototype->Set("upper", new ScriptFunction(WrapScriptFunction(StringUpper)));
		prototype->Set("lower", new ScriptFunction(WrapScriptFunction(StringLower)));
		prototype->Set("split", new ScriptFunction(WrapScriptFunction(StringSplit)));
		prototype->Set("find", new ScriptFunction(WrapScriptFunction(StringFind)));
		prototype->Set("replace", new ScriptFunction(WrapScriptFunction(StringReplace)));
	}

	return prototype;
}

