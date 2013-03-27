/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "config/expression.h"
#include "config/expressionlist.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/array.h"
#include <sstream>
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

Expression::Expression(const String& key, ExpressionOperator op,
    const Value& value, const DebugInfo& debuginfo)
	: m_Key(key), m_Operator(op), m_Value(value), m_DebugInfo(debuginfo)
{
	ASSERT(op != OperatorExecute || value.IsObjectType<ExpressionList>());
}

void Expression::Execute(const Dictionary::Ptr& dictionary) const
{
	Value oldValue, newValue;

	ExpressionList::Ptr valueExprl;
	Dictionary::Ptr valueDict;
	Array::Ptr valueArray;
	if (m_Value.IsObjectType<ExpressionList>())
		valueExprl = m_Value;

	if (m_Value.IsObjectType<Dictionary>())
		valueDict = m_Value;

	if (m_Value.IsObjectType<Array>())
		valueArray = m_Value;

	newValue = m_Value;

	Dictionary::Ptr dict;
	Array::Ptr array;

	switch (m_Operator) {
		case OperatorExecute:
			if (!valueExprl)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Operand for OperatorExecute must be an ExpressionList."));

			valueExprl->Execute(dictionary);

			return;

		case OperatorSet:
			if (valueExprl) {
				dict = boost::make_shared<Dictionary>();
				valueExprl->Execute(dict);
				newValue = dict;
			}

			break;

		case OperatorPlus:
			oldValue = dictionary->Get(m_Key);

			if (oldValue.IsObjectType<Dictionary>())
				dict = oldValue;

			if (oldValue.IsObjectType<Array>())
				array = oldValue;

			if (valueExprl) {
				if (!dict)
					dict = boost::make_shared<Dictionary>();

				valueExprl->Execute(dict);

				newValue = dict;
			} else if (valueDict) {
				if (!dict)
					dict = boost::make_shared<Dictionary>();

				ObjectLock olock(valueDict);

				String key;
				Value value;
				BOOST_FOREACH(boost::tie(key, value), valueDict) {
					dict->Set(key, value);
				}

				newValue = dict;
			} else if (valueArray) {
				if (!array)
					array = boost::make_shared<Array>();


				ObjectLock olock(valueArray);

				BOOST_FOREACH(const Value& value, valueArray) {
					array->Add(value);
				}

				newValue = array;
			} else {
				std::ostringstream message;
				message << "+= only works for dictionaries and arrays ("
					<< m_DebugInfo << ")";
				BOOST_THROW_EXCEPTION(std::invalid_argument(message.str()));
			}

			break;

		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Not yet implemented."));
	}

	dictionary->Set(m_Key, newValue);
}

void Expression::DumpValue(std::ostream& fp, int indent, const Value& value, bool inlineDict)
{
	ExpressionList::Ptr valueExprl;
	Dictionary::Ptr valueDict;
	if (value.IsObjectType<ExpressionList>()) {
		if (!inlineDict)
			fp << "{ " << "\n";

		static_cast<ExpressionList::Ptr>(value)->Dump(fp, indent);

		if (!inlineDict) {
			for (int i = 0; i < indent - 1; i++)
				fp << "\t";

			fp << "}";
		}

		return;
	}

	if (value.IsObjectType<Dictionary>()) {
		if (!inlineDict)
			fp << "{ " << "\n";

		String k;
		Value v;
		BOOST_FOREACH(boost::tie(k, v), static_cast<Dictionary::Ptr>(value)) {
			for (int i = 0; i < indent; i++)
				fp << "\t";

			fp << "\"" << k << "\" = ";
			DumpValue(fp, indent, v);
			fp << "," << "\n";

			fp << "}";
		}

		if (!inlineDict)
			fp << "}";

		return;
	}

	if (value.IsScalar()) {
		fp << "\"" << static_cast<String>(value) << "\"";
		return;
	}

	BOOST_THROW_EXCEPTION(std::runtime_error("Encountered unknown type while dumping value."));
}

void Expression::Dump(std::ostream& fp, int indent) const
{
	if (m_Operator == OperatorExecute) {
		DumpValue(fp, indent, m_Value, true);
		return;
	}

	for (int i = 0; i < indent; i++)
		fp << "\t";

	fp << "\"" << m_Key << "\" ";

	switch (m_Operator) {
		case OperatorSet:
			fp << "=";
			break;
		case OperatorPlus:
			fp << "+=";
			break;
		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Not yet implemented."));
	}

	fp << " ";

	DumpValue(fp, indent + 1, m_Value);

	fp << ", " << "\n";
}

void Expression::ExtractPath(const std::vector<String>& path, const ExpressionList::Ptr& result) const
{
	ASSERT(!path.empty());

	if (path[0] == m_Key) {
		if (!m_Value.IsObjectType<ExpressionList>())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Specified path does not exist."));

		ExpressionList::Ptr exprl = m_Value;

		if (path.size() == 1) {
			result->AddExpression(Expression("", OperatorExecute, exprl, m_DebugInfo));

			return;
		}

		std::vector<String> sub_path(path.begin() + 1, path.end());
		exprl->ExtractPath(sub_path, result);
	} else if (m_Operator == OperatorExecute) {
		ExpressionList::Ptr exprl = m_Value;
		exprl->ExtractPath(path, result);
	}
}

void Expression::ExtractFiltered(const std::set<String, string_iless>& keys, const shared_ptr<ExpressionList>& result) const
{
	if (keys.find(m_Key) != keys.end()) {
		result->AddExpression(*this);
	} else if (m_Operator == OperatorExecute) {
		ExpressionList::Ptr exprl = m_Value;
		exprl->ExtractFiltered(keys, result);
	}
}
