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
#include "config/vmops.hpp"
#include "base/array.hpp"
#include "base/json.hpp"
#include "base/object.hpp"
#include "base/logger.hpp"
#include "base/configerror.hpp"
#include <boost/foreach.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

Expression::~Expression(void)
{ }

Value Expression::Evaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	try {
#ifdef _DEBUG
/*		std::ostringstream msgbuf;
		ShowCodeFragment(msgbuf, GetDebugInfo(), false);
		Log(LogDebug, "Expression")
			<< "Executing:\n" << msgbuf.str();*/
#endif /* _DEBUG */

		return DoEvaluate(context, dhint);
	} catch (const std::exception& ex) {
		if (dynamic_cast<const ConfigError *>(&ex) || boost::get_error_info<boost::errinfo_nested_exception>(ex))
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
	return VMOps::Variable(context, m_Variable);
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
		return false;
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

	std::vector<Value> arguments;
	BOOST_FOREACH(Expression *arg, m_Args) {
		arguments.push_back(arg->Evaluate(context));
	}

	return VMOps::FunctionCall(context, funcName, arguments);
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

		if (VMOps::HasField(acontext, "__result"))
			break;
	}

	Dictionary::Ptr xresult = result->ShallowClone();
	xresult->Remove("__parent");
	return xresult;
}

Value SetExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	DebugHint *psdhint = dhint;
	DebugHint sdhint;

	Value parent, object;
	String index;

	for (Array::SizeType i = 0; i < m_Indexer.size(); i++) {
		Expression *indexExpr = m_Indexer[i];
		String tempindex = indexExpr->Evaluate(context, dhint);

		if (psdhint) {
			sdhint = psdhint->GetChild(tempindex);
			psdhint = &sdhint;
		}

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

		object = VMOps::Indexer(context, parent, tempindex);

		if (i != m_Indexer.size() - 1 && object.IsEmpty()) {
			object = new Dictionary();

			VMOps::SetField(parent, tempindex, object);
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

	VMOps::SetField(parent, index, right);

	if (psdhint)
		psdhint->AddMessage("=", m_DebugInfo);

	return right;
}

Value IndexerExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return VMOps::Indexer(context, m_Operand1->Evaluate(context), m_Operand2->Evaluate(context));
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

Value FunctionExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return VMOps::NewFunction(context, m_Name, m_Args, m_Expression);
}

Value SlotExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return VMOps::NewSlot(context, m_Signal, m_Slot->Evaluate(context));
}

Value ApplyExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	return VMOps::NewApply(context, m_Type, m_Target, m_Name->Evaluate(context), m_Filter, m_FKVar, m_FVVar, m_FTerm, m_Expression, m_DebugInfo);
}

Value ObjectExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	String name;

	if (m_Name)
		name = m_Name->Evaluate(context, dhint);

	return VMOps::NewObject(context, m_Abstract, m_Type, name, m_Filter, m_Zone,
	    m_Expression, m_DebugInfo);
}

Value ForExpression::DoEvaluate(const Object::Ptr& context, DebugHint *dhint) const
{
	Value value = m_Value->Evaluate(context, dhint);

	return VMOps::For(context, m_FKVar, m_FVVar, m_Value->Evaluate(context), m_Expression, m_DebugInfo);
}

