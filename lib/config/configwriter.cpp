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

#include "config/configwriter.hpp"
#include "config/configcompiler.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iterator>

using namespace icinga;

ConfigWriter::ConfigWriter(const String& fileName)
    : m_FP(fileName.CStr(), std::ofstream::out | std::ostream::trunc)
{ }

void ConfigWriter::EmitBoolean(bool val)
{
	m_FP << (val ? "true" : "false");
}

void ConfigWriter::EmitNumber(double val)
{
	m_FP << val;
}

void ConfigWriter::EmitString(const String& val)
{
	m_FP << "\"" << EscapeIcingaString(val) << "\"";
}

void ConfigWriter::EmitEmpty(void)
{
	m_FP << "null";
}

void ConfigWriter::EmitArray(const Array::Ptr& val)
{
	m_FP << "[ ";
	EmitArrayItems(val);
	m_FP << " ]";
}

void ConfigWriter::EmitArrayItems(const Array::Ptr& val)
{
	bool first = true;

	ObjectLock olock(val);
	BOOST_FOREACH(const Value& item, val) {
		if (first)
			first = false;
		else
			m_FP << ", ";

		EmitValue(0, item);
	}
}

void ConfigWriter::EmitScope(int indentLevel, const Dictionary::Ptr& val, const Array::Ptr& imports)
{
	m_FP << "{";

	if (imports && imports->GetLength() > 0) {
		ObjectLock xlock(imports);
		BOOST_FOREACH(const Value& import, imports) {
			m_FP << "\n";
			EmitIndent(indentLevel);
			m_FP << "import \"" << import << "\"";
		}

		m_FP << "\n";
	}

	ObjectLock olock(val);
	BOOST_FOREACH(const Dictionary::Pair& kv, val) {
		m_FP << "\n";
		EmitIndent(indentLevel);
		EmitIdentifier(kv.first, true);
		m_FP << " = ";
		EmitValue(indentLevel + 1, kv.second);
	}

	m_FP << "\n";
	EmitIndent(indentLevel - 1);
	m_FP << "}";
}

void ConfigWriter::EmitValue(int indentLevel, const Value& val)
{
	if (val.IsObjectType<Array>())
		EmitArray(val);
	else if (val.IsObjectType<Dictionary>())
		EmitScope(indentLevel, val);
	else if (val.IsString())
		EmitString(val);
	else if (val.IsNumber())
		EmitNumber(val);
	else if (val.IsBoolean())
		EmitBoolean(val);
	else if (val.IsEmpty())
		EmitEmpty();
}

void ConfigWriter::EmitRaw(const String& val)
{
	m_FP << val;
}

void ConfigWriter::EmitIndent(int indentLevel)
{
	for (int i = 0; i < indentLevel; i++)
		m_FP << "\t";
}

void ConfigWriter::EmitIdentifier(const String& identifier, bool inAssignment)
{
	static std::set<String> keywords;
	if (keywords.empty()) {
		const std::vector<String>& vkeywords = ConfigCompiler::GetKeywords();
		std::copy(vkeywords.begin(), vkeywords.end(), std::inserter(keywords, keywords.begin()));
	}

	if (keywords.find(identifier) != keywords.end()) {
		m_FP << "@" << identifier;
		return;
	}

	boost::regex expr("^[a-zA-Z_][a-zA-Z0-9\\_]*$");
	boost::smatch what;
	if (boost::regex_search(identifier.GetData(), what, expr))
		m_FP << identifier;
	else if (inAssignment)
		EmitString(identifier);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid identifier"));
}

void ConfigWriter::EmitConfigItem(const String& type, const String& name, bool isTemplate,
    const Array::Ptr& imports, const Dictionary::Ptr& attrs)
{
	if (isTemplate)
		m_FP << "template ";
	else
		m_FP << "object ";

	EmitIdentifier(type, false);
	m_FP << " ";
	EmitString(name);
	m_FP << " ";
	EmitScope(1, attrs, imports);
}

void ConfigWriter::EmitComment(const String& text)
{
	m_FP << "/* " << text << " */\n";
}

void ConfigWriter::EmitFunctionCall(const String& name, const Array::Ptr& arguments)
{
	EmitIdentifier(name, false);
	m_FP << "(";
	EmitArrayItems(arguments);
	m_FP << ")";
}

String ConfigWriter::EscapeIcingaString(const String& str)
{
	String result = str;
	boost::algorithm::replace_all(result, "\\", "\\\\");
	boost::algorithm::replace_all(result, "\n", "\\n");
	boost::algorithm::replace_all(result, "\t", "\\t");
	boost::algorithm::replace_all(result, "\r", "\\r");
	boost::algorithm::replace_all(result, "\b", "\\b");
	boost::algorithm::replace_all(result, "\f", "\\f");
	boost::algorithm::replace_all(result, "\"", "\\\"");
	return result;
}
