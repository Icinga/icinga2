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

#include "config/expression.hpp"
#include "config/configitem.hpp"
#include "config/configcompiler.hpp"
#include "config/vmops.hpp"
#include "base/array.hpp"
#include "base/json.hpp"
#include "base/object.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/scriptglobal.hpp"
#include "base/loader.hpp"
#include <boost/exception_ptr.hpp>
#include <boost/exception/errinfo_nested_exception.hpp>

using namespace icinga;

boost::signals2::signal<void (ScriptFrame&, ScriptError *ex, const DebugInfo&)> Expression::OnBreakpoint;
boost::thread_specific_ptr<bool> l_InBreakpointHandler;

Expression::~Expression()
{ }

void Expression::ScriptBreakpoint(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di)
{
	bool *inHandler = l_InBreakpointHandler.get();
	if (!inHandler || !*inHandler) {
		inHandler = new bool(true);
		l_InBreakpointHandler.reset(inHandler);
		OnBreakpoint(frame, ex, di);
		*inHandler = false;
	}
}

ExpressionResult Expression::Evaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	try {
#ifdef I2_DEBUG
/*		std::ostringstream msgbuf;
		ShowCodeLocation(msgbuf, GetDebugInfo(), false);
		Log(LogDebug, "Expression")
			<< "Executing:\n" << msgbuf.str();*/
#endif /* I2_DEBUG */

		frame.IncreaseStackDepth();
		ExpressionResult result = DoEvaluate(frame, dhint);
		frame.DecreaseStackDepth();
		return result;
	} catch (ScriptError& ex) {
		frame.DecreaseStackDepth();

		ScriptBreakpoint(frame, &ex, GetDebugInfo());
		throw;
	} catch (const std::exception& ex) {
		frame.DecreaseStackDepth();

		BOOST_THROW_EXCEPTION(ScriptError("Error while evaluating expression: " + String(ex.what()), GetDebugInfo())
			<< boost::errinfo_nested_exception(boost::current_exception()));
	}

	frame.DecreaseStackDepth();
}

bool Expression::GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const
{
	return false;
}

const DebugInfo& Expression::GetDebugInfo() const
{
	static DebugInfo debugInfo;
	return debugInfo;
}

std::unique_ptr<Expression> icinga::MakeIndexer(ScopeSpecifier scopeSpec, const String& index)
{
	std::unique_ptr<Expression> scope{new GetScopeExpression(scopeSpec)};
	return std::unique_ptr<Expression>(new IndexerExpression(std::move(scope), MakeLiteral(index)));
}

void DictExpression::MakeInline()
{
	m_Inline = true;
}

LiteralExpression::LiteralExpression(const Value& value)
	: m_Value(value)
{ }

ExpressionResult LiteralExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return m_Value;
}

const DebugInfo& DebuggableExpression::GetDebugInfo() const
{
	return m_DebugInfo;
}

ExpressionResult VariableExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value value;

	if (frame.Locals && frame.Locals->Get(m_Variable, &value))
		return value;
	else if (frame.Self.IsObject() && frame.Locals != frame.Self.Get<Object::Ptr>() && frame.Self.Get<Object::Ptr>()->GetOwnField(m_Variable, &value))
		return value;
	else if (VMOps::FindVarImport(frame, m_Variable, &value, m_DebugInfo))
		return value;
	else
		return ScriptGlobal::Get(m_Variable);
}

bool VariableExpression::GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const
{
	*index = m_Variable;

	if (frame.Locals && frame.Locals->Contains(m_Variable)) {
		*parent = frame.Locals;

		if (dhint)
			*dhint = nullptr;
	} else if (frame.Self.IsObject() && frame.Locals != frame.Self.Get<Object::Ptr>() && frame.Self.Get<Object::Ptr>()->HasOwnField(m_Variable)) {
		*parent = frame.Self;

		if (dhint && *dhint)
			*dhint = new DebugHint((*dhint)->GetChild(m_Variable));
	} else if (VMOps::FindVarImportRef(frame, m_Variable, parent, m_DebugInfo)) {
		return true;
	} else if (ScriptGlobal::Exists(m_Variable)) {
		*parent = ScriptGlobal::GetGlobals();

		if (dhint)
			*dhint = nullptr;
	} else
		*parent = frame.Self;

	return true;
}

ExpressionResult NegateExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand = m_Operand->Evaluate(frame);
	CHECK_RESULT(operand);

	return ~(long)operand.GetValue();
}

ExpressionResult LogicalNegateExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand = m_Operand->Evaluate(frame);
	CHECK_RESULT(operand);

	return !operand.GetValue().ToBool();
}

ExpressionResult AddExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() + operand2.GetValue();
}

ExpressionResult SubtractExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() - operand2.GetValue();
}

ExpressionResult MultiplyExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() * operand2.GetValue();
}

ExpressionResult DivideExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() / operand2.GetValue();
}

ExpressionResult ModuloExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() % operand2.GetValue();
}

ExpressionResult XorExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() ^ operand2.GetValue();
}

ExpressionResult BinaryAndExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() & operand2.GetValue();
}

ExpressionResult BinaryOrExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() | operand2.GetValue();
}

ExpressionResult ShiftLeftExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() << operand2.GetValue();
}

ExpressionResult ShiftRightExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() >> operand2.GetValue();
}

ExpressionResult EqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() == operand2.GetValue();
}

ExpressionResult NotEqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() != operand2.GetValue();
}

ExpressionResult LessThanExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() < operand2.GetValue();
}

ExpressionResult GreaterThanExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() > operand2.GetValue();
}

ExpressionResult LessThanOrEqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() <= operand2.GetValue();
}

ExpressionResult GreaterThanOrEqualExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	return operand1.GetValue() >= operand2.GetValue();
}

ExpressionResult InExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	if (operand2.GetValue().IsEmpty())
		return false;
	else if (!operand2.GetValue().IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ScriptError("Invalid right side argument for 'in' operator: " + JsonEncode(operand2.GetValue()), m_DebugInfo));

	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1)

	Array::Ptr arr = operand2.GetValue();
	return arr->Contains(operand1.GetValue());
}

ExpressionResult NotInExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	CHECK_RESULT(operand2);

	if (operand2.GetValue().IsEmpty())
		return true;
	else if (!operand2.GetValue().IsObjectType<Array>())
		BOOST_THROW_EXCEPTION(ScriptError("Invalid right side argument for 'in' operator: " + JsonEncode(operand2.GetValue()), m_DebugInfo));

	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	Array::Ptr arr = operand2.GetValue();
	return !arr->Contains(operand1.GetValue());
}

ExpressionResult LogicalAndExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	if (!operand1.GetValue().ToBool())
		return operand1;
	else {
		ExpressionResult operand2 = m_Operand2->Evaluate(frame);
		CHECK_RESULT(operand2);

		return operand2.GetValue();
	}
}

ExpressionResult LogicalOrExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame);
	CHECK_RESULT(operand1);

	if (operand1.GetValue().ToBool())
		return operand1;
	else {
		ExpressionResult operand2 = m_Operand2->Evaluate(frame);
		CHECK_RESULT(operand2);

		return operand2.GetValue();
	}
}

ExpressionResult FunctionCallExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value self, vfunc;
	String index;

	if (m_FName->GetReference(frame, false, &self, &index))
		vfunc = VMOps::GetField(self, index, frame.Sandboxed, m_DebugInfo);
	else {
		ExpressionResult vfuncres = m_FName->Evaluate(frame);
		CHECK_RESULT(vfuncres);

		vfunc = vfuncres.GetValue();
	}

	if (vfunc.IsObjectType<Type>()) {
		std::vector<Value> arguments;
		arguments.reserve(m_Args.size());
		for (const auto& arg : m_Args) {
			ExpressionResult argres = arg->Evaluate(frame);
			CHECK_RESULT(argres);

			arguments.push_back(argres.GetValue());
		}

		return VMOps::ConstructorCall(vfunc, arguments, m_DebugInfo);
	}

	if (!vfunc.IsObjectType<Function>())
		BOOST_THROW_EXCEPTION(ScriptError("Argument is not a callable object.", m_DebugInfo));

	Function::Ptr func = vfunc;

	if (!func->IsSideEffectFree() && frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Function is not marked as safe for sandbox mode.", m_DebugInfo));

	std::vector<Value> arguments;
	arguments.reserve(m_Args.size());
	for (const auto& arg : m_Args) {
		ExpressionResult argres = arg->Evaluate(frame);
		CHECK_RESULT(argres);

		arguments.push_back(argres.GetValue());
	}

	return VMOps::FunctionCall(frame, self, func, arguments);
}

ExpressionResult ArrayExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Array::Ptr result = new Array();
	result->Reserve(m_Expressions.size());

	for (const auto& aexpr : m_Expressions) {
		ExpressionResult element = aexpr->Evaluate(frame);
		CHECK_RESULT(element);

		result->Add(element.GetValue());
	}

	return result;
}

ExpressionResult DictExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	Value self;

	if (!m_Inline) {
		self = frame.Self;
		frame.Self = new Dictionary();
	}

	Value result;

	try {
		for (const auto& aexpr : m_Expressions) {
			ExpressionResult element = aexpr->Evaluate(frame, m_Inline ? dhint : nullptr);
			CHECK_RESULT(element);
			result = element.GetValue();
		}
	} catch (...) {
		if (!m_Inline)
			std::swap(self, frame.Self);
		throw;
	}

	if (m_Inline)
		return result;
	else {
		std::swap(self, frame.Self);
		return self;
	}
}

ExpressionResult GetScopeExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (m_ScopeSpec == ScopeLocal)
		return frame.Locals;
	else if (m_ScopeSpec == ScopeThis)
		return frame.Self;
	else if (m_ScopeSpec == ScopeGlobal)
		return ScriptGlobal::GetGlobals();
	else
		VERIFY(!"Invalid scope.");
}

ExpressionResult SetExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Assignments are not allowed in sandbox mode.", m_DebugInfo));

	DebugHint *psdhint = dhint;

	Value parent;
	String index;

	if (!m_Operand1->GetReference(frame, true, &parent, &index, &psdhint))
		BOOST_THROW_EXCEPTION(ScriptError("Expression cannot be assigned to.", m_DebugInfo));

	ExpressionResult operand2 = m_Operand2->Evaluate(frame, dhint);
	CHECK_RESULT(operand2);

	if (m_Op != OpSetLiteral) {
		Value object = VMOps::GetField(parent, index, frame.Sandboxed, m_DebugInfo);

		switch (m_Op) {
			case OpSetAdd:
				operand2 = object + operand2;
				break;
			case OpSetSubtract:
				operand2 = object - operand2;
				break;
			case OpSetMultiply:
				operand2 = object * operand2;
				break;
			case OpSetDivide:
				operand2 = object / operand2;
				break;
			case OpSetModulo:
				operand2 = object % operand2;
				break;
			case OpSetXor:
				operand2 = object ^ operand2;
				break;
			case OpSetBinaryAnd:
				operand2 = object & operand2;
				break;
			case OpSetBinaryOr:
				operand2 = object | operand2;
				break;
			default:
				VERIFY(!"Invalid opcode.");
		}
	}

	VMOps::SetField(parent, index, operand2.GetValue(), m_DebugInfo);

	if (psdhint) {
		psdhint->AddMessage("=", m_DebugInfo);

		if (psdhint != dhint)
			delete psdhint;
	}

	return Empty;
}

ExpressionResult ConditionalExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult condition = m_Condition->Evaluate(frame, dhint);
	CHECK_RESULT(condition);

	if (condition.GetValue().ToBool())
		return m_TrueBranch->Evaluate(frame, dhint);
	else if (m_FalseBranch)
		return m_FalseBranch->Evaluate(frame, dhint);

	return Empty;
}

ExpressionResult WhileExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("While loops are not allowed in sandbox mode.", m_DebugInfo));

	for (;;) {
		ExpressionResult condition = m_Condition->Evaluate(frame, dhint);
		CHECK_RESULT(condition);

		if (!condition.GetValue().ToBool())
			break;

		ExpressionResult loop_body = m_LoopBody->Evaluate(frame, dhint);
		CHECK_RESULT_LOOP(loop_body);
	}

	return Empty;
}

ExpressionResult ReturnExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand = m_Operand->Evaluate(frame);
	CHECK_RESULT(operand);

	return ExpressionResult(operand.GetValue(), ResultReturn);
}

ExpressionResult BreakExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return ExpressionResult(Empty, ResultBreak);
}

ExpressionResult ContinueExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return ExpressionResult(Empty, ResultContinue);
}

ExpressionResult IndexerExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult operand1 = m_Operand1->Evaluate(frame, dhint);
	CHECK_RESULT(operand1);

	ExpressionResult operand2 = m_Operand2->Evaluate(frame, dhint);
	CHECK_RESULT(operand2);

	return VMOps::GetField(operand1.GetValue(), operand2.GetValue(), frame.Sandboxed, m_DebugInfo);
}

bool IndexerExpression::GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const
{
	Value vparent;
	String vindex;
	DebugHint *psdhint = nullptr;
	bool free_psd = false;

	if (dhint)
		psdhint = *dhint;

	if (frame.Sandboxed)
		init_dict = false;

	if (m_Operand1->GetReference(frame, init_dict, &vparent, &vindex, &psdhint)) {
		if (init_dict) {
			Value old_value =  VMOps::GetField(vparent, vindex, frame.Sandboxed, m_Operand1->GetDebugInfo());

			if (old_value.IsEmpty() && !old_value.IsString())
				VMOps::SetField(vparent, vindex, new Dictionary(), m_Operand1->GetDebugInfo());
		}

		*parent = VMOps::GetField(vparent, vindex, frame.Sandboxed, m_DebugInfo);
		free_psd = true;
	} else {
		ExpressionResult operand1 = m_Operand1->Evaluate(frame);
		*parent = operand1.GetValue();
	}

	ExpressionResult operand2 = m_Operand2->Evaluate(frame);
	*index = operand2.GetValue();

	if (dhint) {
		if (psdhint)
			*dhint = new DebugHint(psdhint->GetChild(*index));
		else
			*dhint = nullptr;
	}

	if (free_psd)
		delete psdhint;

	return true;
}

void icinga::BindToScope(std::unique_ptr<Expression>& expr, ScopeSpecifier scopeSpec)
{
	DictExpression *dexpr = dynamic_cast<DictExpression *>(expr.get());

	if (dexpr) {
		for (auto& expr : dexpr->m_Expressions)
			BindToScope(expr, scopeSpec);

		return;
	}

	SetExpression *aexpr = dynamic_cast<SetExpression *>(expr.get());

	if (aexpr) {
		BindToScope(aexpr->m_Operand1, scopeSpec);

		return;
	}

	IndexerExpression *iexpr = dynamic_cast<IndexerExpression *>(expr.get());

	if (iexpr) {
		BindToScope(iexpr->m_Operand1, scopeSpec);
		return;
	}

	LiteralExpression *lexpr = dynamic_cast<LiteralExpression *>(expr.get());

	if (lexpr && lexpr->GetValue().IsString()) {
		std::unique_ptr<Expression> scope{new GetScopeExpression(scopeSpec)};
		expr.reset(new IndexerExpression(std::move(scope), std::move(expr), lexpr->GetDebugInfo()));
	}

	VariableExpression *vexpr = dynamic_cast<VariableExpression *>(expr.get());

	if (vexpr) {
		std::unique_ptr<Expression> scope{new GetScopeExpression(scopeSpec)};
		expr.reset(new IndexerExpression(std::move(scope), MakeLiteral(vexpr->GetVariable()), vexpr->GetDebugInfo()));
	}
}

ExpressionResult ThrowExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ExpressionResult messageres = m_Message->Evaluate(frame);
	CHECK_RESULT(messageres);
	Value message = messageres.GetValue();
	BOOST_THROW_EXCEPTION(ScriptError(message, m_DebugInfo, m_IncompleteExpr));
}

ExpressionResult ImportExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Imports are not allowed in sandbox mode.", m_DebugInfo));

	String type = VMOps::GetField(frame.Self, "type", frame.Sandboxed, m_DebugInfo);
	ExpressionResult nameres = m_Name->Evaluate(frame);
	CHECK_RESULT(nameres);
	Value name = nameres.GetValue();

	if (!name.IsString())
		BOOST_THROW_EXCEPTION(ScriptError("Template/object name must be a string", m_DebugInfo));

	ConfigItem::Ptr item = ConfigItem::GetByTypeAndName(Type::GetByName(type), name);

	if (!item)
		BOOST_THROW_EXCEPTION(ScriptError("Import references unknown template: '" + name + "'", m_DebugInfo));

	Dictionary::Ptr scope = item->GetScope();

	if (scope)
		scope->CopyTo(frame.Locals);

	ExpressionResult result = item->GetExpression()->Evaluate(frame, dhint);
	CHECK_RESULT(result);

	return Empty;
}

ExpressionResult ImportDefaultTemplatesExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Imports are not allowed in sandbox mode.", m_DebugInfo));

	String type = VMOps::GetField(frame.Self, "type", frame.Sandboxed, m_DebugInfo);
	Type::Ptr ptype = Type::GetByName(type);

	for (const ConfigItem::Ptr& item : ConfigItem::GetDefaultTemplates(ptype)) {
		Dictionary::Ptr scope = item->GetScope();

		if (scope)
			scope->CopyTo(frame.Locals);

		ExpressionResult result = item->GetExpression()->Evaluate(frame, dhint);
		CHECK_RESULT(result);
	}

	return Empty;
}

ExpressionResult FunctionExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	return VMOps::NewFunction(frame, m_Name, m_Args, m_ClosedVars, m_Expression);
}

ExpressionResult ApplyExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Apply rules are not allowed in sandbox mode.", m_DebugInfo));

	ExpressionResult nameres = m_Name->Evaluate(frame);
	CHECK_RESULT(nameres);

	return VMOps::NewApply(frame, m_Type, m_Target, nameres.GetValue(), m_Filter,
		m_Package, m_FKVar, m_FVVar, m_FTerm, m_ClosedVars, m_IgnoreOnError, m_Expression, m_DebugInfo);
}

ExpressionResult ObjectExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Object definitions are not allowed in sandbox mode.", m_DebugInfo));

	ExpressionResult typeres = m_Type->Evaluate(frame, dhint);
	CHECK_RESULT(typeres);
	Type::Ptr type = typeres.GetValue();

	String name;

	if (m_Name) {
		ExpressionResult nameres = m_Name->Evaluate(frame, dhint);
		CHECK_RESULT(nameres);

		name = nameres.GetValue();
	}

	return VMOps::NewObject(frame, m_Abstract, type, name, m_Filter, m_Zone,
		m_Package, m_DefaultTmpl, m_IgnoreOnError, m_ClosedVars, m_Expression, m_DebugInfo);
}

ExpressionResult ForExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("For loops are not allowed in sandbox mode.", m_DebugInfo));

	ExpressionResult valueres = m_Value->Evaluate(frame, dhint);
	CHECK_RESULT(valueres);

	return VMOps::For(frame, m_FKVar, m_FVVar, valueres.GetValue(), m_Expression, m_DebugInfo);
}

ExpressionResult LibraryExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Loading libraries is not allowed in sandbox mode.", m_DebugInfo));

	ExpressionResult libres = m_Operand->Evaluate(frame, dhint);
	CHECK_RESULT(libres);

	Log(LogNotice, "config")
		<< "Ignoring explicit load request for library \"" << libres << "\".";

	return Empty;
}

ExpressionResult IncludeExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Includes are not allowed in sandbox mode.", m_DebugInfo));

	std::unique_ptr<Expression> expr;
	String name, path, pattern;

	switch (m_Type) {
		case IncludeRegular:
			{
				ExpressionResult pathres = m_Path->Evaluate(frame, dhint);
				CHECK_RESULT(pathres);
				path = pathres.GetValue();
			}

			expr = ConfigCompiler::HandleInclude(m_RelativeBase, path, m_SearchIncludes, m_Zone, m_Package, m_DebugInfo);
			break;

		case IncludeRecursive:
			{
				ExpressionResult pathres = m_Path->Evaluate(frame, dhint);
				CHECK_RESULT(pathres);
				path = pathres.GetValue();
			}

			{
				ExpressionResult patternres = m_Pattern->Evaluate(frame, dhint);
				CHECK_RESULT(patternres);
				pattern = patternres.GetValue();
			}

			expr = ConfigCompiler::HandleIncludeRecursive(m_RelativeBase, path, pattern, m_Zone, m_Package, m_DebugInfo);
			break;

		case IncludeZones:
			{
				ExpressionResult nameres = m_Name->Evaluate(frame, dhint);
				CHECK_RESULT(nameres);
				name = nameres.GetValue();
			}

			{
				ExpressionResult pathres = m_Path->Evaluate(frame, dhint);
				CHECK_RESULT(pathres);
				path = pathres.GetValue();
			}

			{
				ExpressionResult patternres = m_Pattern->Evaluate(frame, dhint);
				CHECK_RESULT(patternres);
				pattern = patternres.GetValue();
			}

			expr = ConfigCompiler::HandleIncludeZones(m_RelativeBase, name, path, pattern, m_Package, m_DebugInfo);
			break;
	}

	ExpressionResult res(Empty);

	try {
		res = expr->Evaluate(frame, dhint);
	} catch (const std::exception&) {
		throw;
	}

	return res;
}

ExpressionResult BreakpointExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	ScriptBreakpoint(frame, nullptr, GetDebugInfo());

	return Empty;
}

ExpressionResult UsingExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	if (frame.Sandboxed)
		BOOST_THROW_EXCEPTION(ScriptError("Using directives are not allowed in sandbox mode.", m_DebugInfo));

	ExpressionResult importres = m_Name->Evaluate(frame);
	CHECK_RESULT(importres);
	Value import = importres.GetValue();

	if (!import.IsObjectType<Dictionary>())
		BOOST_THROW_EXCEPTION(ScriptError("The parameter must resolve to an object of type 'Dictionary'", m_DebugInfo));

	ScriptFrame::AddImport(import);

	return Empty;
}

ExpressionResult TryExceptExpression::DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const
{
	try {
		ExpressionResult tryResult = m_TryBody->Evaluate(frame, dhint);
		CHECK_RESULT(tryResult);
	} catch (const std::exception&) {
		ExpressionResult exceptResult = m_ExceptBody->Evaluate(frame, dhint);
		CHECK_RESULT(exceptResult);
	}

	return Empty;
}

