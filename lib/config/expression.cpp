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

#include "config/expression.hpp"
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
#include "base/configerror.hpp"
#include <boost/foreach.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

Expression::Expression(OpCallback op, const Value& operand1, const DebugInfo& di)
	: m_Operator(op), m_Operand1(operand1), m_Operand2(), m_DebugInfo(di)
{ }

Expression::Expression(OpCallback op, const Value& operand1, const Value& operand2, const DebugInfo& di)
	: m_Operator(op), m_Operand1(operand1), m_Operand2(operand2), m_DebugInfo(di)
{ }

Value Expression::Evaluate(const Dictionary::Ptr& locals, DebugHint *dhint) const
{
	try {
#ifdef _DEBUG
		if (m_Operator != &Expression::OpLiteral) {
			std::ostringstream msgbuf;
			ShowCodeFragment(msgbuf, m_DebugInfo, false);
			Log(LogDebug, "Expression", "Executing:\n" + msgbuf.str());
		}
#endif /* _DEBUG */

		return m_Operator(this, locals, dhint);
	} catch (const std::exception& ex) {
		if (boost::get_error_info<boost::errinfo_nested_exception>(ex))
			throw;
		else
			BOOST_THROW_EXCEPTION(ConfigError("Error while evaluating expression: " + String(ex.what())) << boost::errinfo_nested_exception(boost::current_exception()) << errinfo_debuginfo(m_DebugInfo));
	}
}

void Expression::MakeInline(void)
{
	if (m_Operator == &Expression::OpDict)
		m_Operand2 = true;
}

void Expression::DumpOperand(std::ostream& stream, const Value& operand, int indent) {
	if (operand.IsObjectType<Array>()) {
		Array::Ptr arr = operand;
		stream << String(indent, ' ') << "Array:\n";
		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& elem, arr) {
			DumpOperand(stream, elem, indent + 1);
		}
	} else if (operand.IsObjectType<Expression>()) {
		Expression::Ptr left = operand;
		left->Dump(stream, indent);
	} else {
		stream << String(indent, ' ') << JsonSerialize(operand) << "\n";
	}
}

void Expression::Dump(std::ostream& stream, int indent) const
{
	String sym = Utility::GetSymbolName(reinterpret_cast<const void *>(m_Operator));
	stream << String(indent, ' ') << "op: " << Utility::DemangleSymbolName(sym) << "\n";
	stream << String(indent, ' ') << "left:\n";
	DumpOperand(stream, m_Operand1, indent + 1);
	
	stream << String(indent, ' ') << "right:\n";
	DumpOperand(stream, m_Operand2, indent + 1);
}

Value Expression::EvaluateOperand1(const Dictionary::Ptr& locals, DebugHint *dhint) const
{
	return static_cast<Expression::Ptr>(m_Operand1)->Evaluate(locals, dhint);
}

Value Expression::EvaluateOperand2(const Dictionary::Ptr& locals, DebugHint *dhint) const
{
	return static_cast<Expression::Ptr>(m_Operand2)->Evaluate(locals, dhint);
}

Value Expression::OpLiteral(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->m_Operand1;
}

Value Expression::OpVariable(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Dictionary::Ptr scope = locals;

	while (scope) {
		if (scope->Contains(expr->m_Operand1))
			return scope->Get(expr->m_Operand1);

		scope = scope->Get("__parent");
	}

	return ScriptVariable::Get(expr->m_Operand1);
}

Value Expression::OpNegate(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return ~(long)expr->EvaluateOperand1(locals);
}

Value Expression::OpLogicalNegate(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return !expr->EvaluateOperand1(locals).ToBool();
}

Value Expression::OpAdd(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) + expr->EvaluateOperand2(locals);
}

Value Expression::OpSubtract(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) - expr->EvaluateOperand2(locals);
}

Value Expression::OpMultiply(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) * expr->EvaluateOperand2(locals);
}

Value Expression::OpDivide(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) / expr->EvaluateOperand2(locals);
}

Value Expression::OpBinaryAnd(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) & expr->EvaluateOperand2(locals);
}

Value Expression::OpBinaryOr(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) | expr->EvaluateOperand2(locals);
}

Value Expression::OpShiftLeft(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) << expr->EvaluateOperand2(locals);
}

Value Expression::OpShiftRight(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) >> expr->EvaluateOperand2(locals);
}

Value Expression::OpEqual(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) == expr->EvaluateOperand2(locals);
}

Value Expression::OpNotEqual(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) != expr->EvaluateOperand2(locals);
}

Value Expression::OpLessThan(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) < expr->EvaluateOperand2(locals);
}

Value Expression::OpGreaterThan(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) > expr->EvaluateOperand2(locals);
}

Value Expression::OpLessThanOrEqual(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) <= expr->EvaluateOperand2(locals);
}

Value Expression::OpGreaterThanOrEqual(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals) >= expr->EvaluateOperand2(locals);
}

Value Expression::OpIn(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
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

Value Expression::OpNotIn(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return !OpIn(expr, locals, dhint);
}

Value Expression::OpLogicalAnd(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals).ToBool() && expr->EvaluateOperand2(locals).ToBool();
}

Value Expression::OpLogicalOr(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	return expr->EvaluateOperand1(locals).ToBool() || expr->EvaluateOperand2(locals).ToBool();
}

Value Expression::OpFunctionCall(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
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
		const Expression::Ptr& aexpr = arr->Get(index);
		arguments.push_back(aexpr->Evaluate(locals));
	}

	return func->Invoke(arguments);
}

Value Expression::OpArray(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Array::Ptr arr = expr->m_Operand1;
	Array::Ptr result = make_shared<Array>();

	if (arr) {
		for (Array::SizeType index = 0; index < arr->GetLength(); index++) {
			const Expression::Ptr& aexpr = arr->Get(index);
			result->Add(aexpr->Evaluate(locals));
		}
	}

	return result;
}

Value Expression::OpDict(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Array::Ptr arr = expr->m_Operand1;
	bool in_place = expr->m_Operand2;
	Dictionary::Ptr result = make_shared<Dictionary>();

	result->Set("__parent", locals);

	if (arr) {
		for (Array::SizeType index = 0; index < arr->GetLength(); index++) {
			const Expression::Ptr& aexpr = arr->Get(index);
			Dictionary::Ptr alocals = in_place ? locals : result;
			aexpr->Evaluate(alocals, dhint);

			if (alocals->Contains("__result"))
				break;
		}
	}

	Dictionary::Ptr xresult = result->ShallowClone();
	xresult->Remove("__parent");
	return xresult;
}

Value Expression::OpSet(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Value index = expr->EvaluateOperand1(locals);

	DebugHint *sdhint = NULL;
	if (dhint)
		sdhint = dhint->GetChild(index);

	Value right = expr->EvaluateOperand2(locals, sdhint);
	locals->Set(index, right);

	if (sdhint)
		sdhint->AddMessage("=", expr->m_DebugInfo);

	return right;
}

Value Expression::OpSetPlus(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	Expression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &Expression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	DebugHint *sdhint = NULL;
	if (dhint)
		sdhint = dhint->GetChild(index);

	Value result = left + expr->EvaluateOperand2(xlocals, sdhint);

	if (exp_right->m_Operator == &Expression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);

	if (sdhint)
		sdhint->AddMessage("+=", expr->m_DebugInfo);

	return result;
}

Value Expression::OpSetMinus(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	Expression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &Expression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	DebugHint *sdhint = NULL;
	if (dhint)
		sdhint = dhint->GetChild(index);

	Value result = left - expr->EvaluateOperand2(xlocals, sdhint);

	if (exp_right->m_Operator == &Expression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);

	if (sdhint)
		sdhint->AddMessage("-=", expr->m_DebugInfo);

	return result;
}

Value Expression::OpSetMultiply(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	Expression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &Expression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	DebugHint *sdhint = NULL;
	if (dhint)
		sdhint = dhint->GetChild(index);

	Value result = left * expr->EvaluateOperand2(xlocals, sdhint);

	if (exp_right->m_Operator == &Expression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);

	if (sdhint)
		sdhint->AddMessage("*=", expr->m_DebugInfo);

	return result;
}

Value Expression::OpSetDivide(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Value index = expr->EvaluateOperand1(locals);
	Value left = locals->Get(index);
	Expression::Ptr exp_right = expr->m_Operand2;
	Dictionary::Ptr xlocals = locals;

	if (exp_right->m_Operator == &Expression::OpDict) {
		xlocals = left;

		if (!xlocals)
			xlocals = make_shared<Dictionary>();

		xlocals->Set("__parent", locals);
	}

	DebugHint *sdhint = NULL;
	if (dhint)
		sdhint = dhint->GetChild(index);

	Value result = left / expr->EvaluateOperand2(xlocals, sdhint);

	if (exp_right->m_Operator == &Expression::OpDict) {
		Dictionary::Ptr dict = result;
		dict->Remove("__parent");
	}

	locals->Set(index, result);

	if (sdhint)
		sdhint->AddMessage("/=", expr->m_DebugInfo);

	return result;
}

Value Expression::OpIndexer(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
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

Value Expression::OpImport(const Expression *expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Value type = expr->EvaluateOperand1(locals);
	Value name = expr->EvaluateOperand2(locals);

	ConfigItem::Ptr item = ConfigItem::GetObject(type, name);

	if (!item)
		BOOST_THROW_EXCEPTION(ConfigError("Import references unknown template: '" + name + "'"));

	item->GetExpressionList()->Evaluate(locals, dhint);

	return Empty;
}

Value Expression::FunctionWrapper(const std::vector<Value>& arguments, const Array::Ptr& funcargs, const Expression::Ptr& expr, const Dictionary::Ptr& scope)
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

Value Expression::OpFunction(const Expression* expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Array::Ptr left = expr->m_Operand1;
	Expression::Ptr aexpr = left->Get(1);
	String name = left->Get(0);

	Array::Ptr funcargs = expr->m_Operand2;
	ScriptFunction::Ptr func = make_shared<ScriptFunction>(boost::bind(&Expression::FunctionWrapper, _1, funcargs, aexpr, locals));

	if (!name.IsEmpty())
		ScriptFunction::Register(name, func);

	return func;
}

Value Expression::OpApply(const Expression* expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Array::Ptr left = expr->m_Operand1;
	Expression::Ptr exprl = expr->m_Operand2;
	String type = left->Get(0);
	String target = left->Get(1);
	Expression::Ptr aname = left->Get(2);
	Expression::Ptr filter = left->Get(3);

	String name = aname->Evaluate(locals, dhint);

	ApplyRule::AddRule(type, target, name, exprl, filter, expr->m_DebugInfo, locals);

	return Empty;
}

Value Expression::OpObject(const Expression* expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Array::Ptr left = expr->m_Operand1;
	Expression::Ptr exprl = expr->m_Operand2;
	bool abstract = left->Get(0);
	String type = left->Get(1);
	Expression::Ptr aname = left->Get(2);
	Expression::Ptr filter = left->Get(3);
	String zone = left->Get(4);

	String name = aname->Evaluate(locals, dhint);

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

Value Expression::OpFor(const Expression* expr, const Dictionary::Ptr& locals, DebugHint *dhint)
{
	Array::Ptr left = expr->m_Operand1;
	String varname = left->Get(0);
	Expression::Ptr aexpr = left->Get(1);
	Expression::Ptr ascope = expr->m_Operand2;

	Array::Ptr arr = aexpr->Evaluate(locals, dhint);

	ObjectLock olock(arr);
	BOOST_FOREACH(const Value& value, arr) {
		Dictionary::Ptr xlocals = make_shared<Dictionary>();
		xlocals->Set("__parent", locals);
		xlocals->Set(varname, value);

		ascope->Evaluate(xlocals, dhint);
	}

	return Empty;
}

Dictionary::Ptr DebugHint::ToDictionary(void) const
{
	Dictionary::Ptr result = make_shared<Dictionary>();

	Array::Ptr messages = make_shared<Array>();
	typedef std::pair<String, DebugInfo> MessageType;
	BOOST_FOREACH(const MessageType& message, Messages) {
		Array::Ptr amsg = make_shared<Array>();
		amsg->Add(message.first);
		amsg->Add(message.second.Path);
		amsg->Add(message.second.FirstLine);
		amsg->Add(message.second.FirstColumn);
		amsg->Add(message.second.LastLine);
		amsg->Add(message.second.LastColumn);
		messages->Add(amsg);
	}

	result->Set("messages", messages);

	Dictionary::Ptr properties = make_shared<Dictionary>();

	typedef std::map<String, DebugHint>::value_type ChildType;
	BOOST_FOREACH(const ChildType& kv, Children) {
		properties->Set(kv.first, kv.second.ToDictionary());
	}

	result->Set("properties", properties);

	return result;
}

