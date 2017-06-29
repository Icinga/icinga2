/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/configwriter.hpp"
#include "base/exception.hpp"
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <set>
#include <iterator>

using namespace icinga;

void ConfigWriter::EmitBoolean(std::ostream& fp, bool val)
{
	fp << (val ? "true" : "false");
}

void ConfigWriter::EmitNumber(std::ostream& fp, double val)
{
	fp << std::fixed << val;
}

void ConfigWriter::EmitString(std::ostream& fp, const String& val)
{
	fp << "\"" << EscapeIcingaString(val) << "\"";
}

void ConfigWriter::EmitEmpty(std::ostream& fp)
{
	fp << "null";
}

void ConfigWriter::EmitArray(std::ostream& fp, int indentLevel, const Array::Ptr& val)
{
	fp << "[ ";
	EmitArrayItems(fp, indentLevel, val);
	if (val->GetLength() > 0)
		fp << " ";
	fp << "]";
}

void ConfigWriter::EmitArrayItems(std::ostream& fp, int indentLevel, const Array::Ptr& val)
{
	bool first = true;

	ObjectLock olock(val);
	for (const Value& item : val) {
		if (first)
			first = false;
		else
			fp << ", ";

		EmitValue(fp, indentLevel, item);
	}
}

void ConfigWriter::EmitScope(std::ostream& fp, int indentLevel, const Dictionary::Ptr& val,
    const Array::Ptr& imports, bool splitDot)
{
	fp << "{";

	if (imports && imports->GetLength() > 0) {
		ObjectLock xlock(imports);
		for (const Value& import : imports) {
			fp << "\n";
			EmitIndent(fp, indentLevel);
			fp << "import \"" << import << "\"";
		}

		fp << "\n";
	}

	if (val) {
		ObjectLock olock(val);
		for (const Dictionary::Pair& kv : val) {
			fp << "\n";
			EmitIndent(fp, indentLevel);

			if (splitDot) {
				std::vector<String> tokens;
				boost::algorithm::split(tokens, kv.first, boost::is_any_of("."));

				EmitIdentifier(fp, tokens[0], true);

				for (std::vector<String>::size_type i = 1; i < tokens.size(); i++) {
					fp << "[";
					EmitString(fp, tokens[i]);
					fp << "]";
				}
			} else
				EmitIdentifier(fp, kv.first, true);

			fp << " = ";
			EmitValue(fp, indentLevel + 1, kv.second);
		}
	}

	fp << "\n";
	EmitIndent(fp, indentLevel - 1);
	fp << "}";
}

void ConfigWriter::EmitValue(std::ostream& fp, int indentLevel, const Value& val)
{
	if (val.IsObjectType<Array>())
		EmitArray(fp, indentLevel, val);
	else if (val.IsObjectType<Dictionary>())
		EmitScope(fp, indentLevel, val);
	else if (val.IsObjectType<ConfigIdentifier>())
		EmitIdentifier(fp, static_cast<ConfigIdentifier::Ptr>(val)->GetName(), false);
	else if (val.IsString())
		EmitString(fp, val);
	else if (val.IsNumber())
		EmitNumber(fp, val);
	else if (val.IsBoolean())
		EmitBoolean(fp, val);
	else if (val.IsEmpty())
		EmitEmpty(fp);
}

void ConfigWriter::EmitRaw(std::ostream& fp, const String& val)
{
	fp << val;
}

void ConfigWriter::EmitIndent(std::ostream& fp, int indentLevel)
{
	for (int i = 0; i < indentLevel; i++)
		fp << "\t";
}

void ConfigWriter::EmitIdentifier(std::ostream& fp, const String& identifier, bool inAssignment)
{
	static std::set<String> keywords;
	static boost::mutex mutex;

	{
		boost::mutex::scoped_lock lock(mutex);
		if (keywords.empty()) {
			const std::vector<String>& vkeywords = GetKeywords();
			std::copy(vkeywords.begin(), vkeywords.end(), std::inserter(keywords, keywords.begin()));
		}
	}

	if (keywords.find(identifier) != keywords.end()) {
		fp << "@" << identifier;
		return;
	}

	boost::regex expr("^[a-zA-Z_][a-zA-Z0-9\\_]*$");
	boost::smatch what;
	if (boost::regex_search(identifier.GetData(), what, expr))
		fp << identifier;
	else if (inAssignment)
		EmitString(fp, identifier);
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid identifier"));
}

void ConfigWriter::EmitConfigItem(std::ostream& fp, const String& type, const String& name, bool isTemplate,
    bool ignoreOnError, const Array::Ptr& imports, const Dictionary::Ptr& attrs)
{
	if (isTemplate)
		fp << "template ";
	else
		fp << "object ";

	EmitIdentifier(fp, type, false);
	fp << " ";
	EmitString(fp, name);

	if (ignoreOnError)
		fp << " ignore_on_error";

	fp << " ";
	EmitScope(fp, 1, attrs, imports, true);
}

void ConfigWriter::EmitComment(std::ostream& fp, const String& text)
{
	fp << "/* " << text << " */\n";
}

void ConfigWriter::EmitFunctionCall(std::ostream& fp, const String& name, const Array::Ptr& arguments)
{
	EmitIdentifier(fp, name, false);
	fp << "(";
	EmitArrayItems(fp, 0, arguments);
	fp << ")";
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

const std::vector<String>& ConfigWriter::GetKeywords(void)
{
	static std::vector<String> keywords;
	static boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);

	if (keywords.empty()) {
		keywords.push_back("object");
		keywords.push_back("template");
		keywords.push_back("include");
		keywords.push_back("include_recursive");
		keywords.push_back("include_zones");
		keywords.push_back("library");
		keywords.push_back("null");
		keywords.push_back("true");
		keywords.push_back("false");
		keywords.push_back("const");
		keywords.push_back("var");
		keywords.push_back("this");
		keywords.push_back("globals");
		keywords.push_back("locals");
		keywords.push_back("use");
		keywords.push_back("__using");
		keywords.push_back("default");
		keywords.push_back("ignore_on_error");
		keywords.push_back("current_filename");
		keywords.push_back("current_line");
		keywords.push_back("apply");
		keywords.push_back("to");
		keywords.push_back("where");
		keywords.push_back("import");
		keywords.push_back("assign");
		keywords.push_back("ignore");
		keywords.push_back("function");
		keywords.push_back("return");
		keywords.push_back("break");
		keywords.push_back("continue");
		keywords.push_back("for");
		keywords.push_back("if");
		keywords.push_back("else");
		keywords.push_back("while");
		keywords.push_back("throw");
		keywords.push_back("try");
		keywords.push_back("except");
	}

	return keywords;
}

ConfigIdentifier::ConfigIdentifier(const String& identifier)
    : m_Name(identifier)
{ }

String ConfigIdentifier::GetName(void) const
{
	return m_Name;
}
