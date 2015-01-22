/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include <boost/lexical_cast.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

Expression::~Expression(void)
{ }

Value Expression::Evaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	try {
#ifdef _DEBUG
		std::ostringstream msgbuf;
		ShowCodeFragment(msgbuf, GetDebugInfo(), false);
		Log(LogDebug, "Expression")
			<< "Executing:\n" << msgbuf.str();
#endif /* _DEBUG */

		return DoEvaluate(context, dhint);
	} catch (const std::exception& ex) {
		if (boost::get_error_info<boost::errinfo_nested_exception>(ex))
			throw;
		else
			BOOST_THROW_EXCEPTION(ConfigError("Error while evaluating expression: " + String(ex.what()))
			    << boost::errinfo_nested_exception(boost::current_exception())
			    << errinfo_debuginfo(GetDebugInfo()));
	}
}

const DebugInfo& Expression::GetDebugInfo(void) const
{
	static DebugInfo debugInfo;
	return debugInfo;
}

std::vector<Expression *> icinga::MakeIndexer(const String& index1)
{
	std::vector<Expression *> result;
	result.push_back(MakeLiteral(index1));
	return result;
}

void DictExpression::MakeInline(void)
{
	m_Inline = true;
}

LiteralExpression::LiteralExpression(const Value& value)
	: m_Value(value)
{ }

Value LiteralExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Value;
}

const DebugInfo& DebuggableExpression::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Value VariableExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Object::Ptr scope = context;

	while (scope) {
		if (HasField(scope, m_Variable))
			return GetField(scope, m_Variable);

		scope = GetField(scope, "__parent");
	}

	return ScriptVariable::Get(m_Variable);
}

Value NegateExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return ~(long)m_Operand->Evaluate(context);
}

Value LogicalNegateExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return !m_Operand->Evaluate(context).ToBool();
}

Value AddExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) + m_Operand2->Evaluate(context);
}

Value SubtractExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) - m_Operand2->Evaluate(context);
}

Value MultiplyExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) * m_Operand2->Evaluate(context);
}

Value DivideExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) / m_Operand2->Evaluate(context);
}

Value BinaryAndExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) & m_Operand2->Evaluate(context);
}

Value BinaryOrExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) | m_Operand2->Evaluate(context);
}

Value ShiftLeftExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) << m_Operand2->Evaluate(context);
}

Value ShiftRightExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) >> m_Operand2->Evaluate(context);
}

Value EqualExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) == m_Operand2->Evaluate(context);
}

Value NotEqualExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) != m_Operand2->Evaluate(context);
}

Value LessThanExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) < m_Operand2->Evaluate(context);
}

Value GreaterThanExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) > m_Operand2->Evaluate(context);
}

Value LessThanOrEqualExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) <= m_Operand2->Evaluate(context);
}

Value GreaterThanOrEqualExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context) >= m_Operand2->Evaluate(context);
}

Value InExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value right = m_Operand2->Evaluate(context);

	if (right.IsEmpty())
		return false;
	else if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonEncode(right)));

	Value left = m_Operand1->Evaluate(context);
		
	Array::Ptr arr = right;
	return arr->Contains(left);
}

Value NotInExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value right = m_Operand2->Evaluate(context);

	if (right.IsEmpty())
		return true;
	else if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ConfigError("Invalid right side argument for 'in' operator: " + JsonEncode(right)));

	Value left = m_Operand1->Evaluate(context);

	Array::Ptr arr = right;
	return !arr->Contains(left);
}

Value LogicalAndExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context).ToBool() && m_Operand2->Evaluate(context).ToBool();
}

Value LogicalOrExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(context).ToBool() || m_Operand2->Evaluate(context).ToBool();
}

Value FunctionCallExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value funcName = m_FName->Evaluate(context);

	ScriptFunction::Ptr func;

	if (funcName.IsObjectType<ScriptFunction>())
		func = funcName;
	else
		func = ScriptFunction::GetByName(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(ConfigError("Function '" + funcName + "' does not exist."));

	std::vector<Value> arguments;
	BOOST_FOREACH(Expression *arg, m_Args) {
		arguments.push_back(arg->Evaluate(context));
	}

	return func->Invoke(arguments);
}

Value ArrayExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Array::Ptr result = new Array();

	BOOST_FOREACH(Expression *aexpr, m_Expressions) {
		result->Add(aexpr->Evaluate(context));
	}

	return result;
}

Value DictExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Dictionary::Ptr result = new Dictionary();

	result->Set("__parent", context);

	BOOST_FOREACH(Expression *aexpr, m_Expressions) {
		Object::Ptr acontext = m_Inline ? context : result;
		aexpr->Evaluate(acontext, dhint);

		if (HasField(acontext, "__result"))
			break;
	}

	Dictionary::Ptr xresult = result->ShallowClone();
	xresult->Remove("__parent");
	return xresult;
}

Value SetExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	DebugHint *sdhint = dhint;

	Value parent, object;
	String index;

	for (Array::SizeType i = 0; i < m_Indexer.size(); i++) {
		Expression *indexExpr = m_Indexer[i];
		String tempindex = indexExpr->Evaluate(context, dhint);

		if (sdhint)
			sdhint = sdhint->GetChild(tempindex);

		if (i == 0)
			parent = context;
		else
			parent = object;

		if (i == m_Indexer.size() - 1) {
			index = tempindex;

			/* No need to look up the last indexer's value if this is a direct set */
			if (m_Op == OpSetLiteral)
				break;
		}

		LiteralExpression *eparent = MakeLiteral(parent);
		LiteralExpression *eindex = MakeLiteral(tempindex);

		IndexerExpression eip(eparent, eindex, m_DebugInfo);
		object = eip.Evaluate(context, sdhint);

		if (i != m_Indexer.size() - 1 && object.IsEmpty()) {
			object = new Dictionary();

			SetField(parent, tempindex, object);
		}
	}

	Value right = m_Operand2->Evaluate(context, dhint);

	if (m_Op != OpSetLiteral) {
		Expression *lhs = MakeLiteral(object);
		Expression *rhs = MakeLiteral(right);

		switch (m_Op) {
			case OpSetAdd:
				right = AddExpression(lhs, rhs, m_DebugInfo).Evaluate(context, dhint);
				break;
			case OpSetSubtract:
				right = SubtractExpression(lhs, rhs, m_DebugInfo).Evaluate(context, dhint);
				break;
			case OpSetMultiply:
				right = MultiplyExpression(lhs, rhs, m_DebugInfo).Evaluate(context, dhint);
				break;
			case OpSetDivide:
				right = DivideExpression(lhs, rhs, m_DebugInfo).Evaluate(context, dhint);
				break;
			default:
				VERIFY(!"Invalid opcode.");
		}
	}

	SetField(parent, index, right);

	if (sdhint)
		sdhint->AddMessage("=", m_DebugInfo);

	return right;
}

Value IndexerExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value value = m_Operand1->Evaluate(context);
	Value index = m_Operand2->Evaluate(context);

	if (value.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = value;
		return dict->Get(index);
	} else if (value.IsObjectType<Array>()) {
		Array::Ptr arr = value;
		return arr->Get(index);
	} else if (value.IsObject()) {
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

Value ImportExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value type = m_Type->Evaluate(context);
	Value name = m_Name->Evaluate(context);

	ConfigItem::Ptr item = ConfigItem::GetObject(type, name);

	if (!item)
		BOOST_THROW_EXCEPTION(ConfigError("Import references unknown template: '" + name + "'"));

	item->GetExpression()->Evaluate(context, dhint);

	return Empty;
}

Value Expression::FunctionWrapper(const std::vector<Value>& arguments,
    const std::vector<String>& funcargs, const boost::shared_ptr<Expression>& expr, const Object::Ptr& scope)
{
	if (arguments.size() < funcargs.size())
		BOOST_THROW_EXCEPTION(ConfigError("Too few arguments for function"));

	Dictionary::Ptr context = new Dictionary();
	context->Set("__parent", scope);

	for (std::vector<Value>::size_type i = 0; i < std::min(arguments.size(), funcargs.size()); i++)
		context->Set(funcargs[i], arguments[i]);

	expr->Evaluate(context);
	return context->Get("__result");
}

Value FunctionExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	ScriptFunction::Ptr func = new ScriptFunction(boost::bind(&Expression::FunctionWrapper, _1, m_Args, m_Expression, context));

	if (!m_Name.IsEmpty())
		ScriptFunction::Register(m_Name, func);

	return func;
}

Value ApplyExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	String name = m_Name->Evaluate(context, dhint);

	ApplyRule::AddRule(m_Type, m_Target, name, m_Expression, m_Filter, m_FKVar, m_FVVar, m_FTerm, m_DebugInfo, context);

	return Empty;
}

Value ObjectExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	String name;

	if (m_Name)
		name = m_Name->Evaluate(context, dhint);

	ConfigItemBuilder::Ptr item = new ConfigItemBuilder(m_DebugInfo);

	String checkName = name;

	if (!m_Abstract) {
		Type::Ptr ptype = Type::GetByName(m_Type);

		NameComposer *nc = dynamic_cast<NameComposer *>(ptype.get());

		if (nc)
			checkName = nc->MakeName(name, Dictionary::Ptr());
	}

	if (!checkName.IsEmpty()) {
		ConfigItem::Ptr oldItem = ConfigItem::GetObject(m_Type, checkName);

		if (oldItem) {
			std::ostringstream msgbuf;
			msgbuf << "Object '" << name << "' of type '" << m_Type << "' re-defined: " << m_DebugInfo << "; previous definition: " << oldItem->GetDebugInfo();
			BOOST_THROW_EXCEPTION(ConfigError(msgbuf.str()) << errinfo_debuginfo(m_DebugInfo));
		}
	}

	item->SetType(m_Type);

	if (name.FindFirstOf("!") != String::NPos) {
		std::ostringstream msgbuf;
		msgbuf << "Name for object '" << name << "' of type '" << m_Type << "' is invalid: Object names may not contain '!'";
		BOOST_THROW_EXCEPTION(ConfigError(msgbuf.str()) << errinfo_debuginfo(m_DebugInfo));
	}

	item->SetName(name);

	item->AddExpression(new OwnedExpression(m_Expression));
	item->SetAbstract(m_Abstract);
	item->SetScope(context);
	item->SetZone(m_Zone);
	item->Compile()->Register();

	if (m_Filter)
		ObjectRule::AddRule(m_Type, name, m_Filter, m_DebugInfo, context);

	return Empty;
}

Value ForExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value value = m_Value->Evaluate(context, dhint);

	if (value.IsObjectType<Array>()) {
		if (!m_FVVar.IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Cannot use dictionary iterator for array.") << errinfo_debuginfo(m_DebugInfo));

		Array::Ptr arr = value;

		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& value, arr) {
			Dictionary::Ptr xcontext = new Dictionary();
			xcontext->Set("__parent", context);
			xcontext->Set(m_FKVar, value);

			m_Expression->Evaluate(xcontext, dhint);
		}
	} else if (value.IsObjectType<Dictionary>()) {
		if (m_FVVar.IsEmpty())
			BOOST_THROW_EXCEPTION(ConfigError("Cannot use array iterator for dictionary.") << errinfo_debuginfo(m_DebugInfo));

		Dictionary::Ptr dict = value;

		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			Dictionary::Ptr xcontext = new Dictionary();
			xcontext->Set("__parent", context);
			xcontext->Set(m_FKVar, kv.first);
			xcontext->Set(m_FVVar, kv.second);

			m_Expression->Evaluate(xcontext, dhint);
		}
	} else
		BOOST_THROW_EXCEPTION(ConfigError("Invalid type in __for expression: " + value.GetTypeName()) << errinfo_debuginfo(m_DebugInfo));

	return Empty;
}

Dictionary::Ptr DebugHint::ToDictionary(void) const
{
	Dictionary::Ptr result = new Dictionary();

	Array::Ptr messages = new Array();
	typedef std::pair<String, DebugInfo> MessageType;
	BOOST_FOREACH(const MessageType& message, Messages) {
		Array::Ptr amsg = new Array();
		amsg->Add(message.first);
		amsg->Add(message.second.Path);
		amsg->Add(message.second.FirstLine);
		amsg->Add(message.second.FirstColumn);
		amsg->Add(message.second.LastLine);
		amsg->Add(message.second.LastColumn);
		messages->Add(amsg);
	}

	result->Set("messages", messages);

	Dictionary::Ptr properties = new Dictionary();

	typedef std::map<String, DebugHint>::value_type ChildType;
	BOOST_FOREACH(const ChildType& kv, Children) {
		properties->Set(kv.first, kv.second.ToDictionary());
	}

	result->Set("properties", properties);

	return result;
}

bool Expression::HasField(const Object::Ptr& context, const String& field)
{
	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(context);

	if (dict)
		return dict->Contains(field);
	else {
		Type::Ptr type = context->GetReflectionType();

		if (!type)
			return false;

		return type->GetFieldId(field) != -1;
	}
}

Value Expression::GetField(const Object::Ptr& context, const String& field)
{
	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(context);

	if (dict)
		return dict->Get(field);
	else {
		Type::Ptr type = context->GetReflectionType();

		if (!type)
			return Empty;

		int fid = type->GetFieldId(field);

		if (fid == -1)
			return Empty;

		return context->GetField(fid);
	}
}

void Expression::SetField(const Object::Ptr& context, const String& field, const Value& value)
{
	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(context);

	if (dict)
		dict->Set(field, value);
	else {
		Type::Ptr type = context->GetReflectionType();

		if (!type)
			BOOST_THROW_EXCEPTION(ConfigError("Cannot set field on object."));

		int fid = type->GetFieldId(field);

		if (fid == -1)
			BOOST_THROW_EXCEPTION(ConfigError("Attribute '" + field + "' does not exist."));

		try {
			context->SetField(fid, value);
		} catch (const boost::bad_lexical_cast&) {
			BOOST_THROW_EXCEPTION(ConfigError("Attribute '" + field + "' cannot be set to value of type '" + value.GetTypeName() + "'"));
		} catch (const std::bad_cast&) {
			BOOST_THROW_EXCEPTION(ConfigError("Attribute '" + field + "' cannot be set to value of type '" + value.GetTypeName() + "'"));
		}
	}
}

