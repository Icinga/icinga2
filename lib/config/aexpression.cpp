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

#include "config/aexpression.hpp"
#include "config/configerror.hpp"
#include "config/configitem.hpp"
#include "config/configitembuilder.hpp"
#include "config/applyrule.hpp"
#include "config/objectrule.hpp"
#include "base/array.hpp"
#include "base/serializer.hpp"
#include "base/scriptfunction.hpp"
#include "base/scriptvariable.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/object.hpp"
#include "base/logger_fwd.hpp"
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
#ifdef _DEBUG
		if (m_Operator != &AExpression::OpLiteral) {
			std::ostringstream msgbuf;
			ShowCodeFragment(msgbuf, m_DebugInfo, false);
			Log(LogDebug, "AExpression", "Executing:\n" + msgbuf.str());
		}
#endif /* _DEBUG */

		return m_Operator(this, locals);
	} catch (const std::exception& ex) {
		if (boost::get_error_info<boost::errinfo_nested_exception>(ex))
			throw;
		else
			BOOST_THROW_EXCEPTION(ConfigError("Error while evaluating expression: " + String(ex.what())) << boost::errinfo_nested_exception(boost::current_exception()) << errinfo_debuginfo(m_DebugInfo));
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
		ObjectLock olock(arr);
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

void AExpression::Dump(std::ostream& stream, int indent) const
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

Value AExpression::OpLiteral(const AExpression *expr, const Dictionary::Ptr&)
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

Value AExpression::OpLogicalNegate(const AExpression *expr, const Dictionary::Ptr& locals)
{
	return !expr->EvaluateOperand1(locals).ToBool();
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

	if (right.IsEmpty())
		return false;
	else if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonSerialize(right)));

	Value left = expr->EvaluateOperand1(locals);
		
	Array::Ptr arr = right;
	bool found = false;
	ObjectLock olock(arr);
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
	Value funcName = expr->EvaluateOperand1(locals);

	ScriptFunction::Ptr func;

	if (funcName.IsObjectType<ScriptFunction>())
		func = funcName;
	else
		func = ScriptFunction::GetByName(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(ConfigError("Function '" + funcName + "' does not exist."));

	Array::Ptr arr = expr->EvaluateOperand2(locals);
	std::vector<Value> arguments;
	for (Array::SizeType index = 0; index < arr->GetLength(); index++) {
		const AExpression::Ptr& aexpr = arr->Get(index);
		arguments.push_back(aexpr->Evaluate(locals));
	}

	return func->Invoke(arguments);
}

Value AExpression::OpArray(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Array::Ptr arr = expr->m_Operand1;
	Array::Ptr result = make_shared<Array>();

	if (arr) {
		for (Array::SizeType index = 0; index < arr->GetLength(); index++) {
			const AExpression::Ptr& aexpr = arr->Get(index);
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
		for (Array::SizeType index = 0; index < arr->GetLength(); index++) {
			const AExpression::Ptr& aexpr = arr->Get(index);
			Dictionary::Ptr alocals = in_place ? locals : result;
			aexpr->Evaluate(alocals);

			if (alocals->Contains("__result"))
				break;
		}
	}

	Dictionary::Ptr xresult = result->ShallowClone();
	xresult->Remove("__parent");
	return xresult;
}

Value AExpression::OpSet(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value index = expr->EvaluateOperand1(locals);
	Value right = expr->EvaluateOperand2(locals);
	locals->Set(index, right);
	return right;
}

Value AExpression::OpSetPlus(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	Value result = left + expr->EvaluateOperand2(xlocals);

	if (exp_right->m_Operator == &AExpression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);
	return result;
}

Value AExpression::OpSetMinus(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	Value result = left - expr->EvaluateOperand2(xlocals);

	if (exp_right->m_Operator == &AExpression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);
	return result;
}

Value AExpression::OpSetMultiply(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	Value result = left * expr->EvaluateOperand2(xlocals);

	if (exp_right->m_Operator == &AExpression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);
	return result;
}

Value AExpression::OpSetDivide(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	AExpression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &AExpression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	Value result = left / expr->EvaluateOperand2(xlocals);

	if (exp_right->m_Operator == &AExpression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);
	return result;
}

Value AExpression::OpIndexer(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value value = expr->EvaluateOperand1(locals);
	Value index = expr->EvaluateOperand2(locals);

	if (value.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = value;
		return dict->Get(index);
	} else if (value.IsObjectType<Array>()) {
		Array::Ptr arr = value;
		return arr->Get(index);
	} else if (value.IsObjectType<Object>()) {
		Object::Ptr object = value;
		const Type *type = object->GetReflectionType();

		if (!type)
			BOOST_THROW_EXCEPTION(ConfigError("Dot operator applied to object which does not support reflection"));

		int field = type->GetFieldId(index);

		if (field == -1)
			BOOST_THROW_EXCEPTION(ConfigError("Tried to access invalid property '" + index + "'"));

		return object->GetField(field);
	} else if (value.IsEmpty()) {
		return Empty;
	} else {
		BOOST_THROW_EXCEPTION(ConfigError("Dot operator cannot be applied to type '" + value.GetTypeName() + "'"));
	}
}

Value AExpression::OpImport(const AExpression *expr, const Dictionary::Ptr& locals)
{
	Value type = expr->EvaluateOperand1(locals);
	Value name = expr->EvaluateOperand2(locals);

	ConfigItem::Ptr item = ConfigItem::GetObject(type, name);

	if (!item)
		BOOST_THROW_EXCEPTION(ConfigError("Import references unknown template: '" + name + "'"));

	item->GetExpressionList()->Evaluate(locals);

	return Empty;
}

Value AExpression::FunctionWrapper(const std::vector<Value>& arguments, const Array::Ptr& funcargs, const AExpression::Ptr& expr, const Dictionary::Ptr& scope)
{
	if (arguments.size() < funcargs->GetLength())
		BOOST_THROW_EXCEPTION(ConfigError("Too few arguments for function"));

	Dictionary::Ptr locals = make_shared<Dictionary>();
	locals->Set("__parent", scope);

	for (std::vector<Value>::size_type i = 0; i < std::min(arguments.size(), funcargs->GetLength()); i++)
		locals->Set(funcargs->Get(i), arguments[i]);

	expr->Evaluate(locals);
	return locals->Get("__result");
}

Value AExpression::OpFunction(const AExpression* expr, const Dictionary::Ptr& locals)
{
	Array::Ptr left = expr->m_Operand1;
	AExpression::Ptr aexpr = left->Get(1);
	String name = left->Get(0);

	Array::Ptr funcargs = expr->m_Operand2;
	ScriptFunction::Ptr func = make_shared<ScriptFunction>(boost::bind(&AExpression::FunctionWrapper, _1, funcargs, aexpr, locals));

	if (!name.IsEmpty())
		ScriptFunction::Register(name, func);

	return func;
}

Value AExpression::OpApply(const AExpression* expr, const Dictionary::Ptr& locals)
{
	Array::Ptr left = expr->m_Operand1;
	AExpression::Ptr exprl = expr->m_Operand2;
	String type = left->Get(0);
	String target = left->Get(1);
	AExpression::Ptr aname = left->Get(2);
	AExpression::Ptr filter = left->Get(3);

	String name = aname->Evaluate(locals);

	ApplyRule::AddRule(type, target, name, exprl, filter, expr->m_DebugInfo, locals);

	return Empty;
}

Value AExpression::OpObject(const AExpression* expr, const Dictionary::Ptr& locals)
{
	Array::Ptr left = expr->m_Operand1;
	AExpression::Ptr exprl = expr->m_Operand2;
	bool abstract = left->Get(0);
	String type = left->Get(1);
	AExpression::Ptr aname = left->Get(2);
	AExpression::Ptr filter = left->Get(3);
	String zone = left->Get(4);

	String name = aname->Evaluate(locals);

	ConfigItemBuilder::Ptr item = make_shared<ConfigItemBuilder>(expr->m_DebugInfo);

	String checkName = name;

	if (!abstract) {
		const NameComposer *nc = dynamic_cast<const NameComposer *>(Type::GetByName(type));

		if (nc)
			checkName = nc->MakeName(name, Dictionary::Ptr());
	}

	if (!checkName.IsEmpty()) {
		ConfigItem::Ptr oldItem = ConfigItem::GetObject(type, checkName);

		if (oldItem) {
			std::ostringstream msgbuf;
			msgbuf << "Object '" << name << "' of type '" << type << "' re-defined: " << expr->m_DebugInfo << "; previous definition: " << oldItem->GetDebugInfo();
			BOOST_THROW_EXCEPTION(ConfigError(msgbuf.str()) << errinfo_debuginfo(expr->m_DebugInfo));
		}
	}

	item->SetType(type);

	if (name.FindFirstOf("!") != String::NPos) {
		std::ostringstream msgbuf;
		msgbuf << "Name for object '" << name << "' of type '" << type << "' is invalid: Object names may not contain '!'";
		BOOST_THROW_EXCEPTION(ConfigError(msgbuf.str()) << errinfo_debuginfo(expr->m_DebugInfo));
	}

	item->SetName(name);

	item->AddExpression(exprl);
	item->SetAbstract(abstract);
	item->SetScope(locals);
	item->SetZone(zone);
	item->Compile()->Register();

	ObjectRule::AddRule(type, name, exprl, filter, expr->m_DebugInfo, locals);

	return Empty;
}

Value AExpression::OpFor(const AExpression* expr, const Dictionary::Ptr& locals)
{
	Array::Ptr left = expr->m_Operand1;
	String varname = left->Get(0);
	AExpression::Ptr aexpr = left->Get(1);
	AExpression::Ptr ascope = expr->m_Operand2;

	Array::Ptr arr = aexpr->Evaluate(locals);

	ObjectLock olock(arr);
	BOOST_FOREACH(const Value& value, arr) {
		Dictionary::Ptr xlocals = make_shared<Dictionary>();
		xlocals->Set("__parent", locals);
		xlocals->Set(varname, value);

		ascope->Evaluate(xlocals);
	}

	return Empty;
}
