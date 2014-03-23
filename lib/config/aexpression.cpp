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
#include "base/scriptvariable.h"
#include <boost/foreach.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

AExpression::AExpression(OpCallback op, const Value& operand1, const DebugInfo& di)
	: m_Operator(op), m_Operand1(operand1), m_Operand2(), m_DebugInfo(di)
{ }

AExpression::AExpression(OpCallback op, const Value& operand1, const Value& operand2, const DebugInfo& di)
	: m_Operator(op), m_Operand1(operand1), m_Operand2(operand2), m_DebugInfo(di)
{ }

Value AExpression::Evaluate(const Dictionary::Ptr& locals) const
{
	try {
		return (this->*m_Operator)(locals);
	} catch (const std::exception& ex) {
		if (boost::get_error_info<boost::errinfo_nested_exception>(ex))
			throw;
		else
			BOOST_THROW_EXCEPTION(ConfigError("Error while evaluating expression: " + String(ex.what())) << boost::errinfo_nested_exception(boost::current_exception()) << errinfo_debuginfo(m_DebugInfo));
	}
}

void AExpression::ExtractPath(const std::vector<String>& path, const Array::Ptr& result) const
{
	ASSERT(!path.empty());

	if (m_Operator == &AExpression::OpDict) {
		Array::Ptr exprl = m_Operand1;
		BOOST_FOREACH(const AExpression::Ptr& expr, exprl) {
			expr->ExtractPath(path, result);
		}
	} else if (m_Operator == &AExpression::OpSet && path[0] == m_Operand1) {
		AExpression::Ptr exprl = m_Operand2;

		if (path.size() == 1) {
			result->Add(m_Operand1);

			return;
		}

		std::vector<String> sub_path(path.begin() + 1, path.end());
		exprl->ExtractPath(sub_path, result);
	} else if (m_Operator == &AExpression::OpDict && static_cast<bool>(m_Operand2)) {
		AExpression::Ptr exprl = m_Operand1;
		exprl->ExtractPath(path, result);
	}
}

void AExpression::FindDebugInfoPath(const std::vector<String>& path, DebugInfo& result) const
{
	ASSERT(!path.empty());

	if (m_Operator == &AExpression::OpDict) {
		Array::Ptr exprl = m_Operand1;
		BOOST_FOREACH(const AExpression::Ptr& expr, exprl) {
			expr->FindDebugInfoPath(path, result);
		}
	} else if (m_Operator == &AExpression::OpSet && path[0] == m_Operand1) {
		AExpression::Ptr exprl = m_Operand2;

		if (path.size() == 1) {
			result = m_DebugInfo;
		} else {
			std::vector<String> sub_path(path.begin() + 1, path.end());
			exprl->FindDebugInfoPath(sub_path, result);
		}
	} else if (m_Operator == &AExpression::OpDict && static_cast<bool>(m_Operand2)) {
		AExpression::Ptr exprl = m_Operand1;
		exprl->FindDebugInfoPath(path, result);
	}
}

void AExpression::MakeInline(void)
{
	ASSERT(m_Operator == &AExpression::OpDict);
	m_Operand2 = true;
}

Value AExpression::EvaluateOperand1(const Dictionary::Ptr& locals) const
{
	return static_cast<AExpression::Ptr>(m_Operand1)->Evaluate(locals);
}

Value AExpression::EvaluateOperand2(const Dictionary::Ptr& locals) const
{
	return static_cast<AExpression::Ptr>(m_Operand2)->Evaluate(locals);
}

Value AExpression::OpLiteral(const Dictionary::Ptr& locals) const
{
	return m_Operand1;
}

Value AExpression::OpVariable(const Dictionary::Ptr& locals) const
{
	if (locals && locals->Contains(m_Operand1))
		return locals->Get(m_Operand1);
	else
		return ScriptVariable::Get(m_Operand1);
}

Value AExpression::OpNegate(const Dictionary::Ptr& locals) const
{
	return ~(long)EvaluateOperand1(locals);
}

Value AExpression::OpAdd(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) + EvaluateOperand2(locals);
}

Value AExpression::OpSubtract(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) - EvaluateOperand2(locals);
}

Value AExpression::OpMultiply(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) * EvaluateOperand2(locals);
}

Value AExpression::OpDivide(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) / EvaluateOperand2(locals);
}

Value AExpression::OpBinaryAnd(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) & EvaluateOperand2(locals);
}

Value AExpression::OpBinaryOr(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) | EvaluateOperand2(locals);
}

Value AExpression::OpShiftLeft(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) << EvaluateOperand2(locals);
}

Value AExpression::OpShiftRight(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) >> EvaluateOperand2(locals);
}

Value AExpression::OpEqual(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) == EvaluateOperand2(locals);
}

Value AExpression::OpNotEqual(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) != EvaluateOperand2(locals);
}

Value AExpression::OpLessThan(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) < EvaluateOperand2(locals);
}

Value AExpression::OpGreaterThan(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) > EvaluateOperand2(locals);
}

Value AExpression::OpLessThanOrEqual(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) <= EvaluateOperand2(locals);
}

Value AExpression::OpGreaterThanOrEqual(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals) >= EvaluateOperand2(locals);
}

Value AExpression::OpIn(const Dictionary::Ptr& locals) const
{
	Value right = EvaluateOperand2(locals);

	if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonSerialize(right)));

	Value left = EvaluateOperand1(locals);
		
	Array::Ptr arr = right;
	bool found = false;
	BOOST_FOREACH(const Value& value, arr) {
		if (value == left) {
			found = true;
			break;
		}
	}

	return found;
}

Value AExpression::OpNotIn(const Dictionary::Ptr& locals) const
{
	return !OpIn(locals);
}

Value AExpression::OpLogicalAnd(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals).ToBool() && EvaluateOperand2(locals).ToBool();
}

Value AExpression::OpLogicalOr(const Dictionary::Ptr& locals) const
{
	return EvaluateOperand1(locals).ToBool() || EvaluateOperand2(locals).ToBool();
}

Value AExpression::OpFunctionCall(const Dictionary::Ptr& locals) const
{
	String funcName = m_Operand1;
	ScriptFunction::Ptr func = ScriptFunctionRegistry::GetInstance()->GetItem(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(ConfigError("Function '" + funcName + "' does not exist."));

	Array::Ptr arr = EvaluateOperand2(locals);
	std::vector<Value> arguments;
	BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
		arguments.push_back(aexpr->Evaluate(locals));
	}

	return func->Invoke(arguments);
}

Value AExpression::OpArray(const Dictionary::Ptr& locals) const
{
	Array::Ptr arr = m_Operand1;
	Array::Ptr result = make_shared<Array>();

	if (arr) {
		BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
			result->Add(aexpr->Evaluate(locals));
		}
	}

	return result;
}

Value AExpression::OpDict(const Dictionary::Ptr& locals) const
{
	Array::Ptr arr = m_Operand1;
	bool in_place = m_Operand2;
	Dictionary::Ptr result = make_shared<Dictionary>();

	if (arr) {
		BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
			aexpr->Evaluate(in_place ? locals : result);
		}
	}

	return result;
}

Value AExpression::OpSet(const Dictionary::Ptr& locals) const
{
	Value right = EvaluateOperand2(locals);
	locals->Set(m_Operand1, right);
	return right;
}

Value AExpression::OpSetPlus(const Dictionary::Ptr& locals) const
{
	Value result = locals->Get(m_Operand1) + EvaluateOperand2(locals);
	locals->Set(m_Operand1, result);
	return result;
}

Value AExpression::OpSetMinus(const Dictionary::Ptr& locals) const
{
	Value result = locals->Get(m_Operand1) - EvaluateOperand2(locals);
	locals->Set(m_Operand1, result);
	return result;
}

Value AExpression::OpSetMultiply(const Dictionary::Ptr& locals) const
{
	Value result = locals->Get(m_Operand1) * EvaluateOperand2(locals);
	locals->Set(m_Operand1, result);
	return result;
}

Value AExpression::OpSetDivide(const Dictionary::Ptr& locals) const
{
	Value result = locals->Get(m_Operand1) / EvaluateOperand2(locals);
	locals->Set(m_Operand1, result);
	return result;
}
