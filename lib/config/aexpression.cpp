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
#include "base/utility.h"
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
		return m_Operator(this, locals);
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
	} else if ((m_Operator == &AExpression::OpSet || m_Operator == &AExpression::OpSetPlus ||
	    m_Operator == &AExpression::OpSetMinus || m_Operator == &AExpression::OpSetMultiply ||
	    m_Operator == &AExpression::OpSetDivide) && path[0] == m_Operand1) {
		AExpression::Ptr exprl = m_Operand2;

		if (path.size() == 1) {
			VERIFY(exprl->m_Operator == &AExpression::OpDict);
			Array::Ptr subexprl = exprl->m_Operand1;
			BOOST_FOREACH(const AExpression::Ptr& expr, subexprl) {
				result->Add(expr);
			}

			return;
		}

		std::vector<String> sub_path(path.begin() + 1, path.end());
		exprl->ExtractPath(sub_path, result);
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
	} else if ((m_Operator == &AExpression::OpSet || m_Operator == &AExpression::OpSetPlus ||
	    m_Operator == &AExpression::OpSetMinus || m_Operator == &AExpression::OpSetMultiply ||
	    m_Operator == &AExpression::OpSetDivide) && path[0] == m_Operand1) {
		AExpression::Ptr exprl = m_Operand2;

		if (path.size() == 1) {
			result = m_DebugInfo;
		} else {
			std::vector<String> sub_path(path.begin() + 1, path.end());
			exprl->FindDebugInfoPath(sub_path, result);
		}
	}
}

void AExpression::MakeInline(void)
{
	if (m_Operator == &AExpression::OpDict)
		m_Operand2 = true;
}

void AExpression::DumpOperand(std::ostream& stream, const Value& operand, int indent) {
	if (operand.IsObjectType<Array>()) {
		Array::Ptr arr = operand;
		stream << String(indent, ' ') << "Array:\n";
		BOOST_FOREACH(const Value& elem, arr) {
			DumpOperand(stream, elem, indent + 1);
		}
	} else if (operand.IsObjectType<AExpression>()) {
		AExpression::Ptr left = operand;
		left->Dump(stream, indent);
	} else {
		stream << String(indent, ' ') << JsonSerialize(operand) << "\n";
	}
}

void AExpression::Dump(std::ostream& stream, int indent)
{
	String sym = Utility::GetSymbolName(reinterpret_cast<const void *>(m_Operator));
	stream << String(indent, ' ') << "op: " << Utility::DemangleSymbolName(sym) << "\n";
	stream << String(indent, ' ') << "left:\n";
	DumpOperand(stream, m_Operand1, indent + 1);
	
	stream << String(indent, ' ') << "right:\n";
	DumpOperand(stream, m_Operand2, indent + 1);
}

Value AExpression::EvaluateOperand1(const Dictionary::Ptr& locals) const
{
	return static_cast<AExpression::Ptr>(m_Operand1)->Evaluate(locals);
}

Value AExpression::EvaluateOperand2(const Dictionary::Ptr& locals) const
{
	return static_cast<AExpression::Ptr>(m_Operand2)->Evaluate(locals);
}

Value AExpression::OpLiteral(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->m_Operand1;
}

Value AExpression::OpVariable(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Dictionary::Ptr scope = locals;

	while (scope) {
		if (scope->Contains(expr->m_Operand1))
			return scope->Get(expr->m_Operand1);

		scope = scope->Get("__parent");
	}

	return ScriptVariable::Get(expr->m_Operand1);
}

Value AExpression::OpNegate(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return ~(long)expr->EvaluateOperand1(locals);
}

Value AExpression::OpAdd(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) + expr->EvaluateOperand2(locals);
}

Value AExpression::OpSubtract(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) - expr->EvaluateOperand2(locals);
}

Value AExpression::OpMultiply(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) * expr->EvaluateOperand2(locals);
}

Value AExpression::OpDivide(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) / expr->EvaluateOperand2(locals);
}

Value AExpression::OpBinaryAnd(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) & expr->EvaluateOperand2(locals);
}

Value AExpression::OpBinaryOr(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) | expr->EvaluateOperand2(locals);
}

Value AExpression::OpShiftLeft(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) << expr->EvaluateOperand2(locals);
}

Value AExpression::OpShiftRight(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) >> expr->EvaluateOperand2(locals);
}

Value AExpression::OpEqual(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) == expr->EvaluateOperand2(locals);
}

Value AExpression::OpNotEqual(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) != expr->EvaluateOperand2(locals);
}

Value AExpression::OpLessThan(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) < expr->EvaluateOperand2(locals);
}

Value AExpression::OpGreaterThan(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) > expr->EvaluateOperand2(locals);
}

Value AExpression::OpLessThanOrEqual(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) <= expr->EvaluateOperand2(locals);
}

Value AExpression::OpGreaterThanOrEqual(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals) >= expr->EvaluateOperand2(locals);
}

Value AExpression::OpIn(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value right = expr->EvaluateOperand2(locals);

	if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonSerialize(right)));

	Value left = expr->EvaluateOperand1(locals);
		
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

Value AExpression::OpNotIn(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return !OpIn(expr, locals);
}

Value AExpression::OpLogicalAnd(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals).ToBool() && expr->EvaluateOperand2(locals).ToBool();
}

Value AExpression::OpLogicalOr(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return expr->EvaluateOperand1(locals).ToBool() || expr->EvaluateOperand2(locals).ToBool();
}

Value AExpression::OpFunctionCall(const AExpression *expr, const Dictionary::Ptr& locals)
{
	String funcName = expr->m_Operand1;
	ScriptFunction::Ptr func = ScriptFunctionRegistry::GetInstance()->GetItem(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(ConfigError("Function '" + funcName + "' does not exist."));

	Array::Ptr arr = expr->EvaluateOperand2(locals);
	std::vector<Value> arguments;
	BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
		arguments.push_back(aexpr->Evaluate(locals));
	}

	return func->Invoke(arguments);
}

Value AExpression::OpArray(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Array::Ptr arr = expr->m_Operand1;
	Array::Ptr result = make_shared<Array>();

	if (arr) {
		BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
			result->Add(aexpr->Evaluate(locals));
		}
	}

	return result;
}

Value AExpression::OpDict(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Array::Ptr arr = expr->m_Operand1;
	bool in_place = expr->m_Operand2;
	Dictionary::Ptr result = make_shared<Dictionary>();

	result->Set("__parent", locals);

	if (arr) {
		BOOST_FOREACH(const AExpression::Ptr& aexpr, arr) {
			aexpr->Evaluate(in_place ? locals : result);
		}
	}

	result->Remove("__parent");

	return result;
}

Value AExpression::OpSet(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value right = expr->EvaluateOperand2(locals);
	locals->Set(expr->m_Operand1, right);
	return right;
}

Value AExpression::OpSetPlus(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value left = locals->Get(expr->m_Operand1);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();
	}

	Value result = left + expr->EvaluateOperand2(xlocals);
	locals->Set(expr->m_Operand1, result);
	return result;
}

Value AExpression::OpSetMinus(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value left = locals->Get(expr->m_Operand1);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();
	}

	Value result = left - expr->EvaluateOperand2(xlocals);
	locals->Set(expr->m_Operand1, result);
	return result;
}

Value AExpression::OpSetMultiply(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value left = locals->Get(expr->m_Operand1);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();
	}

	Value result = left * expr->EvaluateOperand2(xlocals);
	locals->Set(expr->m_Operand1, result);
	return result;
}

Value AExpression::OpSetDivide(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value left = locals->Get(expr->m_Operand1);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();
	}

	Value result = left / expr->EvaluateOperand2(xlocals);
	locals->Set(expr->m_Operand1, result);
	return result;
}

Value AExpression::OpIndexer(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Dictionary::Ptr dict = locals->Get(expr->m_Operand1);
	
	if (!dict)
		BOOST_THROW_EXCEPTION(ConfigError("Script variable '" + expr->m_Operand1 + "' not set in this scope."));
	
	return dict->Get(expr->m_Operand2);
}
