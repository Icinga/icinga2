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

#include "config/aexpression.h"
#include "config/configerror.h"
#include "base/array.h"
#include "base/serializer.h"
#include "base/context.h"
#include "base/scriptfunction.h"
#include <boost/foreach.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

AExpression::AExpression(AOperator op, const AValue& operand1, const DebugInfo& di)
	: m_Operator(op), m_Operand1(operand1), m_DebugInfo(di)
{
	ASSERT(op == AEReturn);
}

AExpression::AExpression(AOperator op, const AValue& operand1, const AValue& operand2, const DebugInfo& di)
	: m_Operator(op), m_Operand1(operand1), m_Operand2(operand2), m_DebugInfo(di)
{
	ASSERT(op == AEAdd || op == AENegate || op == AESubtract || op == AEMultiply || op == AEDivide ||
		op == AEBinaryAnd || op == AEBinaryOr || op == AEShiftLeft || op == AEShiftRight ||
		op == AEEqual || op == AENotEqual || op == AEIn || op == AENotIn ||
		op == AELogicalAnd || op == AELogicalOr || op == AEFunctionCall);
}

Value AExpression::Evaluate(const Dictionary::Ptr& locals) const
{
	Value left, right;
	Array::Ptr arr, arr2;
	bool found;
	String funcName;
	ScriptFunction::Ptr func;
	std::vector<Value> arguments;

	left = m_Operand1.Evaluate(locals);
	right = m_Operand2.Evaluate(locals);

	std::ostringstream msgbuf;
	msgbuf << "Evaluating AExpression " << m_DebugInfo << "; left=" << JsonSerialize(left) << "; right=" << JsonSerialize(right);
	CONTEXT(msgbuf.str());

	try {
		switch (m_Operator) {
			case AEReturn:
				return left;
			case AENegate:
				return ~(long)left;
			case AEAdd:
				return left + right;
			case AESubtract:
				return left - right;
			case AEMultiply:
				return left * right;
			case AEDivide:
				return left / right;
			case AEBinaryAnd:
				return left & right;
			case AEBinaryOr:
				return left | right;
			case AEShiftLeft:
				return left << right;
			case AEShiftRight:
				return left >> right;
			case AEEqual:
				return left == right;
			case AENotEqual:
				return left != right;
			case AELessThan:
				return left < right;
			case AEGreaterThan:
				return left > right;
			case AELessThanOrEqual:
				return left <= right;
			case AEGreaterThanOrEqual:
				return left >= right;
			case AEIn:
				if (!right.IsObjectType<Array>())
					BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonSerialize(right)));

				arr = right;
				found = false;
				BOOST_FOREACH(const Value& value, arr) {
					if (value == left) {
						found = true;
						break;
					}
				}

				return found;
			case AENotIn:
				if (!right.IsObjectType<Array>())
					BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonSerialize(right)));

				arr = right;
				found = false;
				BOOST_FOREACH(const Value& value, arr) {
					if (value == left) {
						found = true;
						break;
					}
				}

				return !found;
			case AELogicalAnd:
				return left.ToBool() && right.ToBool();
			case AELogicalOr:
				return left.ToBool() || right.ToBool();
			case AEFunctionCall:
				funcName = left;
				func = ScriptFunctionRegistry::GetInstance()->GetItem(funcName);

				if (!func)
					BOOST_THROW_EXCEPTION(ConfigError("Function '" + funcName + "' does not exist."));

				arr = right;
				BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
					arguments.push_back(aexpr->Evaluate(locals));
				}

				return func->Invoke(arguments);
			case AEArray:
				arr = left;
				arr2 = make_shared<Array>();

				if (arr) {
					BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
						arr2->Add(aexpr->Evaluate(locals));
					}
				}

				return arr2;
			default:
				ASSERT(!"Invalid operator.");
		}
	} catch (const std::exception& ex) {
		if (boost::get_error_info<boost::errinfo_nested_exception>(ex))
			throw;
		else
			BOOST_THROW_EXCEPTION(ConfigError("Error while evaluating expression.") << boost::errinfo_nested_exception(boost::current_exception()) << errinfo_debuginfo(m_DebugInfo));
	}
}
