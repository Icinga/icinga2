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
#include "base/json.hpp"
#include "base/scriptfunction.hpp"
#include "base/scriptvariable.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/object.hpp"
#include "base/logger.hpp"
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
			Log(LogDebug, "Expression")
			    << "Executing:\n" << msgbuf.str();
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
		stream << String(indent, ' ') << JsonEncode(operand) << "\n";
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
		BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonEncode(right)));

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
	Array::Ptr left = expr->m_Operand1;
	Array::Ptr indexer = left->Get(0);
	int csop = left->Get(1);

	DebugHint *sdhint = dhint;

	Value parent, object;
	String index;

	for (Array::SizeType i = 0; i < indexer->GetLength(); i++) {
		Expression::Ptr indexExpr = indexer->Get(i);
		String tempindex = indexExpr->Evaluate(locals, dhint);

		if (i == indexer->GetLength() - 1)
			index = tempindex;

		if (i == 0) {
			parent = locals;
			object = locals->Get(tempindex);
		} else {
			parent = object;

			Expression::Ptr eparent = make_shared<Expression>(&Expression::OpLiteral, parent, expr->m_DebugInfo);
			Expression::Ptr eindex = make_shared<Expression>(&Expression::OpLiteral, tempindex, expr->m_DebugInfo);

			Expression::Ptr eip = make_shared<Expression>(&Expression::OpIndexer, eparent, eindex, expr->m_DebugInfo);
			object = eip->Evaluate(locals, dhint);
		}

		if (sdhint)
			sdhint = sdhint->GetChild(index);

		if (i != indexer->GetLength() - 1 && object.IsEmpty()) {
			object = make_shared<Dictionary>();

			Dictionary::Ptr pdict = parent;
			pdict->Set(tempindex, object);
		}
	}

	Value right = expr->EvaluateOperand2(locals, dhint);

	if (csop != OpSetLiteral) {
		Expression::OpCallback op;

		switch (csop) {
			case OpSetAdd:
				op = &Expression::OpAdd;
				break;
			case OpSetSubtract:
				op = &Expression::OpSubtract;
				break;
			case OpSetMultiply:
				op = &Expression::OpMultiply;
				break;
			case OpSetDivide:
				op = &Expression::OpDivide;
				break;
			default:
				VERIFY(!"Invalid opcode.");
		}

		Expression::Ptr ecp = make_shared<Expression>(op,
		    make_shared<Expression>(&Expression::OpLiteral, object, expr->m_DebugInfo),
		    make_shared<Expression>(&Expression::OpLiteral, right, expr->m_DebugInfo),
		    expr->m_DebugInfo);

		right = ecp->Evaluate(locals, dhint);
	}

	Dictionary::Ptr pdict = parent;
	pdict->Set(index, right);

	if (sdhint)
		sdhint->AddMessage("=", expr->m_DebugInfo);

	return right;
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
		Type::Ptr type = object->GetReflectionType();

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
	String fkvar = left->Get(4);
	String fvvar = left->Get(5);
	Expression::Ptr fterm = left->Get(6);

	String name = aname->Evaluate(locals, dhint);

	ApplyRule::AddRule(type, target, name, exprl, filter, fkvar, fvvar, fterm, expr->m_DebugInfo, locals);

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
		shared_ptr<NameComposer> nc = dynamic_pointer_cast<NameComposer>(Type::GetByName(type));

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
	String kvar = left->Get(0);
	String vvar = left->Get(1);
	Expression::Ptr aexpr = left->Get(2);
	Expression::Ptr ascope = expr->m_Operand2;

	Value value = aexpr->Evaluate(locals, dhint);

	if (value.IsObjectType<Array>()) {
		if (!vvar.IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Cannot use dictionary iterator for array.") << errinfo_debuginfo(expr->m_DebugInfo));

		Array::Ptr arr = value;

		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& value, arr) {
			Dictionary::Ptr xlocals = make_shared<Dictionary>();
			xlocals->Set("__parent", locals);
			xlocals->Set(kvar, value);

			ascope->Evaluate(xlocals, dhint);
		}
	} else if (value.IsObjectType<Dictionary>()) {
		if (vvar.IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Cannot use array iterator for dictionary.") << errinfo_debuginfo(expr->m_DebugInfo));

		Dictionary::Ptr dict = value;

		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			Dictionary::Ptr xlocals = make_shared<Dictionary>();
			xlocals->Set("__parent", locals);
			xlocals->Set(kvar, kv.first);
			xlocals->Set(vvar, kv.second);

			ascope->Evaluate(xlocals, dhint);
		}
	} else
		BOOST_THROW_EXCEPTION(ConfigError("Invalid type in __for expression: " + value.GetTypeName()) << errinfo_debuginfo(expr->m_DebugInfo));

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

Expression::Ptr icinga::MakeLiteral(const Value& lit)
{
	return make_shared<Expression>(&Expression::OpLiteral, lit, DebugInfo());
}
