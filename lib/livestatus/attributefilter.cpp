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

#include "livestatus/attributefilter.hpp"
#include "base/convert.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace icinga;

AttributeFilter::AttributeFilter(String column, String op, String operand)
	: m_Column(std::move(column)), m_Operator(std::move(op)), m_Operand(std::move(operand))
{ }

bool AttributeFilter::Apply(const Table::Ptr& table, const Value& row)
{
	Column column = table->GetColumn(m_Column);

	Value value = column.ExtractValue(row);

	if (value.IsObjectType<Array>()) {
		Array::Ptr array = value;

		if (m_Operator == ">=" || m_Operator == "<") {
			bool negate = (m_Operator == "<");

			ObjectLock olock(array);
			for (const String& item : array) {
				if (item == m_Operand)
					return !negate; /* Item found in list. */
			}

			return negate; /* Item not found in list. */
		} else if (m_Operator == "=") {
			return (array->GetLength() == 0);
		} else {
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid operator for column '" + m_Column + "': " + m_Operator + " (expected '>=' or '=')."));
		}
	} else {
		if (m_Operator == "=") {
			if (value.GetType() == ValueNumber || value.GetType() == ValueBoolean)
				return (static_cast<double>(value) == Convert::ToDouble(m_Operand));
			else
				return (static_cast<String>(value) == m_Operand);
		} else if (m_Operator == "~") {
			bool ret;
			try {
				boost::regex expr(m_Operand.GetData());
				String operand = value;
				boost::smatch what;
				ret = boost::regex_search(operand.GetData(), what, expr);
			} catch (boost::exception&) {
				Log(LogWarning, "AttributeFilter")
					<< "Regex '" << m_Operand << " " << m_Operator << " " << value << "' error.";
				ret = false;
			}

			//Log(LogDebug, "LivestatusListener/AttributeFilter")
			//    << "Attribute filter '" << m_Operand + " " << m_Operator << " "
			//    << value << "' " << (ret ? "matches" : "doesn't match") << ".";

			return ret;
		} else if (m_Operator == "=~") {
			bool ret;
			try {
				String operand = value;
				ret = boost::iequals(operand, m_Operand.GetData());
			} catch (boost::exception&) {
				Log(LogWarning, "AttributeFilter")
					<< "Case-insensitive equality '" << m_Operand << " " << m_Operator << " " << value << "' error.";
				ret = false;
			}

			return ret;
		} else if (m_Operator == "~~") {
			bool ret;
			try {
				boost::regex expr(m_Operand.GetData(), boost::regex::icase);
				String operand = value;
				boost::smatch what;
				ret = boost::regex_search(operand.GetData(), what, expr);
			} catch (boost::exception&) {
				Log(LogWarning, "AttributeFilter")
					<< "Regex '" << m_Operand << " " << m_Operator << " " << value << "' error.";
				ret = false;
			}

			//Log(LogDebug, "LivestatusListener/AttributeFilter")
			//    << "Attribute filter '" << m_Operand << " " << m_Operator << " "
			//    << value << "' " << (ret ? "matches" : "doesn't match") << ".";

			return ret;
		} else if (m_Operator == "<") {
			if (value.GetType() == ValueNumber)
				return (static_cast<double>(value) < Convert::ToDouble(m_Operand));
			else
				return (static_cast<String>(value) < m_Operand);
		} else if (m_Operator == ">") {
			if (value.GetType() == ValueNumber)
				return (static_cast<double>(value) > Convert::ToDouble(m_Operand));
			else
				return (static_cast<String>(value) > m_Operand);
		} else if (m_Operator == "<=") {
			if (value.GetType() == ValueNumber)
				return (static_cast<double>(value) <= Convert::ToDouble(m_Operand));
			else
				return (static_cast<String>(value) <= m_Operand);
		} else if (m_Operator == ">=") {
			if (value.GetType() == ValueNumber)
				return (static_cast<double>(value) >= Convert::ToDouble(m_Operand));
			else
				return (static_cast<String>(value) >= m_Operand);
		} else {
			BOOST_THROW_EXCEPTION(std::invalid_argument("Unknown operator for column '" + m_Column + "': " + m_Operator));
		}
	}

	return false;
}
