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
#include "base/exception.hpp"
#include "base/scriptglobal.hpp"
#include <boost/foreach.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

Expression::~Expression(void)
{ }

Value Expression::Evaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	try {
#ifdef I2_DEBUG
/*		std::ostringstream msgbuf;
		ShowCodeFragment(msgbuf, GetDebugInfo(), false);
		Log(LogDebug, "Expression")
			<< "Executing:\n" << msgbuf.str();*/
#endif /* I2_DEBUG */

		return DoEvaluate(frame, dhint);
	} catch (const InterruptExecutionError&) {
		throw;
	} catch (const ScriptError& ex) {
		throw;
	} catch (const std::exception& ex) {
		BOOST_THROW_EXCEPTION(ScriptError("Error while evaluating expression: " + String(ex.what()), GetDebugInfo())
		    << boost::errinfo_nested_exception(boost::current_exception()));
	}
}

bool Expression::GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const
{
	return false;
}

const DebugInfo& Expression::GetDebugInfo(void) const
{
	static DebugInfo debugInfo;
	return debugInfo;
}

Expression *icinga::MakeIndexer(ScopeSpecifier scopeSpec, const String& index)
{
	Expression *scope = new GetScopeExpression(scopeSpec);
	return new IndexerExpression(scope, MakeLiteral(index));
}

void DictExpression::MakeInline(void)
{
	m_Inline = true;
}

LiteralExpression::LiteralExpression(const Value& value)
	: m_Value(value)
{ }

Value LiteralExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Value;
}

const DebugInfo& DebuggableExpression::GetDebugInfo(void) const
{
	return m_DebugInfo;
}

Value VariableExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Locals && frame.Locals->Contains(m_Variable))
		return frame.Locals->Get(m_Variable);
	else if (frame.Self.IsObject() && frame.Locals != static_cast<Object::Ptr>(frame.Self) && VMOps::HasField(frame.Self, m_Variable))
		return VMOps::GetField(frame.Self, m_Variable, m_DebugInfo);
	else
		return ScriptGlobal::Get(m_Variable);
}

bool VariableExpression::GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const
{
	*index = m_Variable;

	if (frame.Locals && frame.Locals->Contains(m_Variable)) {
		*parent = frame.Locals;

		if (dhint)
			*dhint = NULL;
	} else if (frame.Self.IsObject() && frame.Locals != static_cast<Object::Ptr>(frame.Self) && VMOps::HasField(frame.Self, m_Variable)) {
		*parent = frame.Self;

		if (dhint && *dhint)
			*dhint = new DebugHint((*dhint)->GetChild(m_Variable));
	} else if (ScriptGlobal::Exists(m_Variable)) {
		*parent = ScriptGlobal::GetGlobals();

		if (dhint)
			*dhint = NULL;
	} else
		*parent = frame.Self;

	return true;
}

Value NegateExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return ~(long)m_Operand->Evaluate(frame);
}

Value LogicalNegateExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return !m_Operand->Evaluate(frame).ToBool();
}

Value AddExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) + m_Operand2->Evaluate(frame);
}

Value SubtractExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) - m_Operand2->Evaluate(frame);
}

Value MultiplyExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) * m_Operand2->Evaluate(frame);
}

Value DivideExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) / m_Operand2->Evaluate(frame);
}

Value ModuloExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) % m_Operand2->Evaluate(frame);
}

Value XorExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) ^ m_Operand2->Evaluate(frame);
}

Value BinaryAndExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) & m_Operand2->Evaluate(frame);
}

Value BinaryOrExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) | m_Operand2->Evaluate(frame);
}

Value ShiftLeftExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) << m_Operand2->Evaluate(frame);
}

Value ShiftRightExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) >> m_Operand2->Evaluate(frame);
}

Value EqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) == m_Operand2->Evaluate(frame);
}

Value NotEqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) != m_Operand2->Evaluate(frame);
}

Value LessThanExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) < m_Operand2->Evaluate(frame);
}

Value GreaterThanExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) > m_Operand2->Evaluate(frame);
}

Value LessThanOrEqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) <= m_Operand2->Evaluate(frame);
}

Value GreaterThanOrEqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame) >= m_Operand2->Evaluate(frame);
}

Value InExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value right = m_Operand2->Evaluate(frame);

	if (right.IsEmpty())
		return false;
	else if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ScriptError("Invalid right side argument for 'in' operator: " + JsonEncode(right), m_DebugInfo));

	Value left = m_Operand1->Evaluate(frame);

	Array::Ptr arr = right;
	return arr->Contains(left);
}

Value NotInExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value right = m_Operand2->Evaluate(frame);

	if (right.IsEmpty())
		return true;
	else if (!right.IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ScriptError("Invalid right side argument for 'in' operator: " + JsonEncode(right), m_DebugInfo));

	Value left = m_Operand1->Evaluate(frame);

	Array::Ptr arr = right;
	return !arr->Contains(left);
}

Value LogicalAndExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame).ToBool() && m_Operand2->Evaluate(frame).ToBool();
}

Value LogicalOrExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Operand1->Evaluate(frame).ToBool() || m_Operand2->Evaluate(frame).ToBool();
}

Value FunctionCallExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value self, vfunc;
	String index;

	if (m_FName->GetReference(frame, false, &self, &index))
		vfunc = VMOps::GetField(self, index, m_DebugInfo);
	else
		vfunc = m_FName->Evaluate(frame);

	if (!vfunc.IsObjectType<Function>())
		BOOST_THROW_EXCEPTION(ScriptError("Argument is not a callable object.", m_DebugInfo));

	Function::Ptr func = vfunc;

	std::vector<Value> arguments;
	BOOST_FOREACH(Expression *arg, m_Args) {
		arguments.push_back(arg->Evaluate(frame));
	}

	return VMOps::FunctionCall(frame, self, func, arguments);
}

Value ArrayExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Array::Ptr result = new Array();

	BOOST_FOREACH(Expression *aexpr, m_Expressions) {
		result->Add(aexpr->Evaluate(frame));
	}

	return result;
}

Value DictExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ScriptFrame *dframe;
	ScriptFrame rframe;

	if (!m_Inline) {
		dframe = &rframe;
		rframe.Locals = frame.Locals;
		rframe.Self = new Dictionary();
	} else {
		dframe = &frame;
	}

	Value result;

	BOOST_FOREACH(Expression *aexpr, m_Expressions) {
		result = aexpr->Evaluate(*dframe, dhint);
	}

	if (m_Inline)
		return result;
	else
		return dframe->Self;
}

Value GetScopeExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (m_ScopeSpec == ScopeLocal)
		return frame.Locals;
	else if (m_ScopeSpec == ScopeCurrent)
		return frame.Self;
	else if (m_ScopeSpec == ScopeThis)
		return frame.Self;
	else if (m_ScopeSpec == ScopeGlobal)
		return ScriptGlobal::GetGlobals();
	else
		ASSERT(!"Invalid scope.");
}

Value SetExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	DebugHint *psdhint = dhint;

	Value parent;
	String index;

	if (!m_Operand1->GetReference(frame, true, &parent, &index, &psdhint))
		BOOST_THROW_EXCEPTION(ScriptError("Expression cannot be assigned to.", m_DebugInfo));

	Value right = m_Operand2->Evaluate(frame, dhint);

	if (m_Op != OpSetLiteral) {
		Value object = VMOps::GetField(parent, index, m_DebugInfo);

		Expression *lhs = MakeLiteral(object);
		Expression *rhs = MakeLiteral(right);

		switch (m_Op) {
			case OpSetAdd:
				right = AddExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetSubtract:
				right = SubtractExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetMultiply:
				right = MultiplyExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetDivide:
				right = DivideExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetModulo:
				right = ModuloExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetXor:
				right = XorExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetBinaryAnd:
				right = BinaryAndExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			case OpSetBinaryOr:
				right = BinaryOrExpression(lhs, rhs, m_DebugInfo).Evaluate(frame, dhint);
				break;
			default:
				VERIFY(!"Invalid opcode.");
		}
	}

	VMOps::SetField(parent, index, right, m_DebugInfo);

	if (psdhint) {
		psdhint->AddMessage("=", m_DebugInfo);

		if (psdhint != dhint)
			delete psdhint;
	}

	return Empty;
}

Value ConditionalExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (m_Condition->Evaluate(frame, dhint).ToBool())
		return m_TrueBranch->Evaluate(frame, dhint);
	else if (m_FalseBranch)
		return m_FalseBranch->Evaluate(frame, dhint);

	return Empty;
}

Value ReturnExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	BOOST_THROW_EXCEPTION(InterruptExecutionError(m_Operand->Evaluate(frame)));
}

Value IndexerExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value object = m_Operand1->Evaluate(frame, dhint);
	String index = m_Operand2->Evaluate(frame, dhint);
	return VMOps::GetField(object, index, m_DebugInfo);
}

bool IndexerExpression::GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const
{
	Value vparent;
	String vindex;

	if (m_Operand1->GetReference(frame, init_dict, &vparent, &vindex, dhint)) {
		if (init_dict && VMOps::GetField(vparent, vindex, m_Operand1->GetDebugInfo()).IsEmpty())
			VMOps::SetField(vparent, vindex, new Dictionary(), m_Operand1->GetDebugInfo());

		*parent = VMOps::GetField(vparent, vindex, m_DebugInfo);
	} else
		*parent = m_Operand1->Evaluate(frame);

	*index = m_Operand2->Evaluate(frame);

	if (dhint && *dhint)
		*dhint = new DebugHint((*dhint)->GetChild(*index));

	return true;
}

void icinga::BindToScope(Expression *& expr, ScopeSpecifier scopeSpec)
{
	DictExpression *dexpr = dynamic_cast<DictExpression *>(expr);

	if (dexpr) {
		BOOST_FOREACH(Expression *& expr, dexpr->m_Expressions)
			BindToScope(expr, scopeSpec);

		return;
	}

	SetExpression *aexpr = dynamic_cast<SetExpression *>(expr);

	if (aexpr) {
		BindToScope(aexpr->m_Operand1, scopeSpec);

		return;
	}

	IndexerExpression *iexpr = dynamic_cast<IndexerExpression *>(expr);

	if (iexpr) {
		BindToScope(iexpr->m_Operand1, scopeSpec);
		return;
	}

	LiteralExpression *lexpr = dynamic_cast<LiteralExpression *>(expr);
	ScriptFrame frame;

	if (lexpr && lexpr->Evaluate(frame).IsString()) {
		Expression *scope = new GetScopeExpression(scopeSpec);
		expr = new IndexerExpression(scope, lexpr, lexpr->GetDebugInfo());
	}

	VariableExpression *vexpr = dynamic_cast<VariableExpression *>(expr);

	if (vexpr) {
		Expression *scope = new GetScopeExpression(scopeSpec);
		Expression *new_expr = new IndexerExpression(scope, MakeLiteral(vexpr->GetVariable()), vexpr->GetDebugInfo());
		delete expr;
		expr = new_expr;
	}
}

Value ImportExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	String type = VMOps::GetField(frame.Self, "type", m_DebugInfo);
	Value name = m_Name->Evaluate(frame);

	if (!name.IsString())
		BOOST_THROW_EXCEPTION(ScriptError("Template/object name must be a string", m_DebugInfo));

	ConfigItem::Ptr item = ConfigItem::GetObject(type, name);

	if (!item)
		BOOST_THROW_EXCEPTION(ScriptError("Import references unknown template: '" + name + "'", m_DebugInfo));

	item->GetExpression()->Evaluate(frame, dhint);

	return Empty;
}

Value FunctionExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return VMOps::NewFunction(frame, m_Args, m_ClosedVars, m_Expression);
}

Value ApplyExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return VMOps::NewApply(frame, m_Type, m_Target, m_Name->Evaluate(frame), m_Filter,
	    m_FKVar, m_FVVar, m_FTerm, m_ClosedVars, m_Expression, m_DebugInfo);
}

Value ObjectExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	String name;

	if (m_Name)
		name = m_Name->Evaluate(frame, dhint);

	return VMOps::NewObject(frame, m_Abstract, m_Type, name, m_Filter, m_Zone,
	    m_ClosedVars, m_Expression, m_DebugInfo);
}

Value ForExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value value = m_Value->Evaluate(frame, dhint);

	return VMOps::For(frame, m_FKVar, m_FVVar, m_Value->Evaluate(frame), m_Expression, m_DebugInfo);
}

