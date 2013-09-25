/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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
#include "base/debug.h"
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
		case OperatorNop:
			/* Nothing to do here. */

			return;

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

void Expression::ErasePath(const std::vector<String>& path)
{
	ASSERT(!path.empty());

	if (path[0] == m_Key) {
		if (path.size() == 1) {
			m_Operator = OperatorNop;
		} else if (m_Value.IsObjectType<ExpressionList>()) {
			ExpressionList::Ptr exprl = m_Value;

			std::vector<String> sub_path(path.begin() + 1, path.end());
			exprl->ErasePath(sub_path);
		}
	} else if (m_Operator == OperatorExecute) {
		ExpressionList::Ptr exprl = m_Value;
		exprl->ErasePath(path);
	}
}

void Expression::FindDebugInfoPath(const std::vector<String>& path, DebugInfo& result) const
{
	ASSERT(!path.empty());

	if (path[0] == m_Key) {
		if (path.size() == 1) {
			result = m_DebugInfo;
		} else if (m_Value.IsObjectType<ExpressionList>()) {
			ExpressionList::Ptr exprl = m_Value;

			std::vector<String> sub_path(path.begin() + 1, path.end());
			exprl->FindDebugInfoPath(sub_path, result);
		}
	} else if (m_Operator == OperatorExecute) {
		ExpressionList::Ptr exprl = m_Value;
		exprl->FindDebugInfoPath(path, result);
	}
}
