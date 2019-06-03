/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "config/i2-config.hpp"
#include "base/debuginfo.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"
#include "base/scriptframe.hpp"
#include "base/convert.hpp"
#include <map>

namespace icinga
{

struct DebugHint
{
public:
	DebugHint(Dictionary::Ptr hints = nullptr)
		: m_Hints(std::move(hints))
	{ }

	DebugHint(Dictionary::Ptr&& hints)
		: m_Hints(std::move(hints))
	{ }

	void AddMessage(const String& message, const DebugInfo& di)
	{
		GetMessages()->Add(new Array({ message, di.Path, di.FirstLine, di.FirstColumn, di.LastLine, di.LastColumn }));
	}

	DebugHint GetChild(const String& name)
	{
		const Dictionary::Ptr& children = GetChildren();

		Value vchild;
		Dictionary::Ptr child;

		if (!children->Get(name, &vchild)) {
			child = new Dictionary();
			children->Set(name, child);
		} else
			child = vchild;

		return DebugHint(child);
	}

	Dictionary::Ptr ToDictionary() const
	{
		return m_Hints;
	}

private:
	Dictionary::Ptr m_Hints;
	Array::Ptr m_Messages;
	Dictionary::Ptr m_Children;

	const Array::Ptr& GetMessages()
	{
		if (m_Messages)
			return m_Messages;

		if (!m_Hints)
			m_Hints = new Dictionary();

		Value vmessages;

		if (!m_Hints->Get("messages", &vmessages)) {
			m_Messages = new Array();
			m_Hints->Set("messages", m_Messages);
		} else
			m_Messages = vmessages;

		return m_Messages;
	}

	const Dictionary::Ptr& GetChildren()
	{
		if (m_Children)
			return m_Children;

		if (!m_Hints)
			m_Hints = new Dictionary();

		Value vchildren;

		if (!m_Hints->Get("properties", &vchildren)) {
			m_Children = new Dictionary();
			m_Hints->Set("properties", m_Children);
		} else
			m_Children = vchildren;

		return m_Children;
	}
};

enum CombinedSetOp
{
	OpSetLiteral,
	OpSetAdd,
	OpSetSubtract,
	OpSetMultiply,
	OpSetDivide,
	OpSetModulo,
	OpSetXor,
	OpSetBinaryAnd,
	OpSetBinaryOr
};

enum ScopeSpecifier
{
	ScopeLocal,
	ScopeThis,
	ScopeGlobal
};

typedef std::map<String, String> DefinitionMap;

/**
 * @ingroup config
 */
enum ExpressionResultCode
{
	ResultOK,
	ResultReturn,
	ResultContinue,
	ResultBreak
};

/**
 * @ingroup config
 */
struct ExpressionResult
{
public:
	template<typename T>
	ExpressionResult(T value, ExpressionResultCode code = ResultOK)
		: m_Value(std::move(value)), m_Code(code)
	{ }

	operator const Value&() const
	{
		return m_Value;
	}

	const Value& GetValue() const
	{
		return m_Value;
	}

	ExpressionResultCode GetCode() const
	{
		return m_Code;
	}

private:
	Value m_Value;
	ExpressionResultCode m_Code;
};

#define CHECK_RESULT(res)			\
	do {					\
		if (res.GetCode() != ResultOK)	\
			return res;		\
	} while (0);

#define CHECK_RESULT_LOOP(res)			\
	if (res.GetCode() == ResultReturn)	\
		return res;			\
	if (res.GetCode() == ResultContinue)	\
		continue;			\
	if (res.GetCode() == ResultBreak)	\
		break;				\

/**
 * @ingroup config
 */
class Expression
{
public:
	Expression() = default;
	Expression(const Expression&) = delete;
	virtual ~Expression();

	Expression& operator=(const Expression&) = delete;

	ExpressionResult Evaluate(ScriptFrame& frame, DebugHint *dhint = nullptr) const;
	virtual bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint = nullptr) const;
	virtual const DebugInfo& GetDebugInfo() const;

	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const = 0;

	static boost::signals2::signal<void (ScriptFrame& frame, ScriptError *ex, const DebugInfo& di)> OnBreakpoint;

	static void ScriptBreakpoint(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di);
};

std::unique_ptr<Expression> MakeIndexer(ScopeSpecifier scopeSpec, const String& index);

class OwnedExpression final : public Expression
{
public:
	OwnedExpression(std::shared_ptr<Expression> expression)
		: m_Expression(std::move(expression))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override
	{
		return m_Expression->DoEvaluate(frame, dhint);
	}

	const DebugInfo& GetDebugInfo() const override
	{
		return m_Expression->GetDebugInfo();
	}

private:
	std::shared_ptr<Expression> m_Expression;
};

class LiteralExpression final : public Expression
{
public:
	LiteralExpression(Value value = Value());

	const Value& GetValue() const
	{
		return m_Value;
	}

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Value m_Value;
};

inline LiteralExpression *MakeLiteralRaw(const Value& literal = Value())
{
	return new LiteralExpression(literal);
}

inline std::unique_ptr<LiteralExpression> MakeLiteral(const Value& literal = Value())
{
	return std::unique_ptr<LiteralExpression>(MakeLiteralRaw(literal));
}

class DebuggableExpression : public Expression
{
public:
	DebuggableExpression(DebugInfo debugInfo = DebugInfo())
		: m_DebugInfo(std::move(debugInfo))
	{ }

protected:
	const DebugInfo& GetDebugInfo() const final;

	DebugInfo m_DebugInfo;
};

class UnaryExpression : public DebuggableExpression
{
public:
	UnaryExpression(std::unique_ptr<Expression> operand, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Operand(std::move(operand))
	{ }

protected:
	std::unique_ptr<Expression> m_Operand;
};

class BinaryExpression : public DebuggableExpression
{
public:
	BinaryExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Operand1(std::move(operand1)), m_Operand2(std::move(operand2))
	{ }

protected:
	std::unique_ptr<Expression> m_Operand1;
	std::unique_ptr<Expression> m_Operand2;
};

class VariableExpression final : public DebuggableExpression
{
public:
	VariableExpression(String variable, std::vector<std::shared_ptr<Expression> > imports, const DebugInfo& debugInfo = DebugInfo());

	String GetVariable() const
	{
		return m_Variable;
	}

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
	bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const override;

private:
	String m_Variable;
	std::vector<std::shared_ptr<Expression> > m_Imports;

	friend void BindToScope(std::unique_ptr<Expression>& expr, ScopeSpecifier scopeSpec);
};

class DerefExpression final : public UnaryExpression
{
public:
	DerefExpression(std::unique_ptr<Expression> operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(operand), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
	bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const override;
};

class RefExpression final : public UnaryExpression
{
public:
	RefExpression(std::unique_ptr<Expression> operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(operand), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class NegateExpression final : public UnaryExpression
{
public:
	NegateExpression(std::unique_ptr<Expression> operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(operand), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class LogicalNegateExpression final : public UnaryExpression
{
public:
	LogicalNegateExpression(std::unique_ptr<Expression> operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(operand), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class AddExpression final : public BinaryExpression
{
public:
	AddExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class SubtractExpression final : public BinaryExpression
{
public:
	SubtractExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class MultiplyExpression final : public BinaryExpression
{
public:
	MultiplyExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class DivideExpression final : public BinaryExpression
{
public:
	DivideExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class ModuloExpression final : public BinaryExpression
{
public:
	ModuloExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class XorExpression final : public BinaryExpression
{
public:
	XorExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class BinaryAndExpression final : public BinaryExpression
{
public:
	BinaryAndExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class BinaryOrExpression final : public BinaryExpression
{
public:
	BinaryOrExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class ShiftLeftExpression final : public BinaryExpression
{
public:
	ShiftLeftExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class ShiftRightExpression final : public BinaryExpression
{
public:
	ShiftRightExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class EqualExpression final : public BinaryExpression
{
public:
	EqualExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class NotEqualExpression final : public BinaryExpression
{
public:
	NotEqualExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class LessThanExpression final : public BinaryExpression
{
public:
	LessThanExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class GreaterThanExpression final : public BinaryExpression
{
public:
	GreaterThanExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class LessThanOrEqualExpression final : public BinaryExpression
{
public:
	LessThanOrEqualExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class GreaterThanOrEqualExpression final : public BinaryExpression
{
public:
	GreaterThanOrEqualExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class InExpression final : public BinaryExpression
{
public:
	InExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class NotInExpression final : public BinaryExpression
{
public:
	NotInExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class LogicalAndExpression final : public BinaryExpression
{
public:
	LogicalAndExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class LogicalOrExpression final : public BinaryExpression
{
public:
	LogicalOrExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class FunctionCallExpression final : public DebuggableExpression
{
public:
	FunctionCallExpression(std::unique_ptr<Expression> fname, std::vector<std::unique_ptr<Expression> >&& args, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_FName(std::move(fname)), m_Args(std::move(args))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

public:
	std::unique_ptr<Expression> m_FName;
	std::vector<std::unique_ptr<Expression> > m_Args;
};

class ArrayExpression final : public DebuggableExpression
{
public:
	ArrayExpression(std::vector<std::unique_ptr<Expression > >&& expressions, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Expressions(std::move(expressions))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::vector<std::unique_ptr<Expression> > m_Expressions;
};

class DictExpression final : public DebuggableExpression
{
public:
	DictExpression(std::vector<std::unique_ptr<Expression> >&& expressions = {}, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Expressions(std::move(expressions))
	{ }

	void MakeInline();

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::vector<std::unique_ptr<Expression> > m_Expressions;
	bool m_Inline{false};

	friend void BindToScope(std::unique_ptr<Expression>& expr, ScopeSpecifier scopeSpec);
};

class SetConstExpression final : public UnaryExpression
{
public:
	SetConstExpression(const String& name, std::unique_ptr<Expression> operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(operand), debugInfo), m_Name(name)
	{ }

protected:
	String m_Name;

	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class SetExpression final : public BinaryExpression
{
public:
	SetExpression(std::unique_ptr<Expression> operand1, CombinedSetOp op, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo), m_Op(op)
	{ }

	void SetOverrideFrozen();

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	CombinedSetOp m_Op;
	bool m_OverrideFrozen{false};

	friend void BindToScope(std::unique_ptr<Expression>& expr, ScopeSpecifier scopeSpec);
};

class ConditionalExpression final : public DebuggableExpression
{
public:
	ConditionalExpression(std::unique_ptr<Expression> condition, std::unique_ptr<Expression> true_branch, std::unique_ptr<Expression> false_branch, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Condition(std::move(condition)), m_TrueBranch(std::move(true_branch)), m_FalseBranch(std::move(false_branch))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::unique_ptr<Expression> m_Condition;
	std::unique_ptr<Expression> m_TrueBranch;
	std::unique_ptr<Expression> m_FalseBranch;
};

class WhileExpression final : public DebuggableExpression
{
public:
	WhileExpression(std::unique_ptr<Expression> condition, std::unique_ptr<Expression> loop_body, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Condition(std::move(condition)), m_LoopBody(std::move(loop_body))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::unique_ptr<Expression> m_Condition;
	std::unique_ptr<Expression> m_LoopBody;
};


class ReturnExpression final : public UnaryExpression
{
public:
	ReturnExpression(std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(expression), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class BreakExpression final : public DebuggableExpression
{
public:
	BreakExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class ContinueExpression final : public DebuggableExpression
{
public:
	ContinueExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class GetScopeExpression final : public Expression
{
public:
	GetScopeExpression(ScopeSpecifier scopeSpec)
		: m_ScopeSpec(scopeSpec)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	ScopeSpecifier m_ScopeSpec;
};

class IndexerExpression final : public BinaryExpression
{
public:
	IndexerExpression(std::unique_ptr<Expression> operand1, std::unique_ptr<Expression> operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(std::move(operand1), std::move(operand2), debugInfo)
	{ }

	void SetOverrideFrozen();

protected:
	bool m_OverrideFrozen{false};

	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
	bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const override;

	friend void BindToScope(std::unique_ptr<Expression>& expr, ScopeSpecifier scopeSpec);
};

void BindToScope(std::unique_ptr<Expression>& expr, ScopeSpecifier scopeSpec);

class ThrowExpression final : public DebuggableExpression
{
public:
	ThrowExpression(std::unique_ptr<Expression> message, bool incompleteExpr, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Message(std::move(message)), m_IncompleteExpr(incompleteExpr)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::unique_ptr<Expression> m_Message;
	bool m_IncompleteExpr;
};

class ImportExpression final : public DebuggableExpression
{
public:
	ImportExpression(std::unique_ptr<Expression> name, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Name(std::move(name))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::unique_ptr<Expression> m_Name;
};

class ImportDefaultTemplatesExpression final : public DebuggableExpression
{
public:
	ImportDefaultTemplatesExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class FunctionExpression final : public DebuggableExpression
{
public:
	FunctionExpression(String name, std::vector<String> args,
		std::map<String, std::unique_ptr<Expression> >&& closedVars, std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Name(std::move(name)), m_Args(std::move(args)), m_ClosedVars(std::move(closedVars)), m_Expression(std::move(expression))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_Name;
	std::vector<String> m_Args;
	std::map<String, std::unique_ptr<Expression> > m_ClosedVars;
	std::shared_ptr<Expression> m_Expression;
};

class ApplyExpression final : public DebuggableExpression
{
public:
	ApplyExpression(String type, String target, std::unique_ptr<Expression> name,
		std::unique_ptr<Expression> filter, String package, String fkvar, String fvvar,
		std::unique_ptr<Expression> fterm, std::map<String, std::unique_ptr<Expression> >&& closedVars, bool ignoreOnError,
		std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Type(std::move(type)), m_Target(std::move(target)),
			m_Name(std::move(name)), m_Filter(std::move(filter)), m_Package(std::move(package)), m_FKVar(std::move(fkvar)), m_FVVar(std::move(fvvar)),
			m_FTerm(std::move(fterm)), m_IgnoreOnError(ignoreOnError), m_ClosedVars(std::move(closedVars)),
			m_Expression(std::move(expression))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_Type;
	String m_Target;
	std::unique_ptr<Expression> m_Name;
	std::shared_ptr<Expression> m_Filter;
	String m_Package;
	String m_FKVar;
	String m_FVVar;
	std::shared_ptr<Expression> m_FTerm;
	bool m_IgnoreOnError;
	std::map<String, std::unique_ptr<Expression> > m_ClosedVars;
	std::shared_ptr<Expression> m_Expression;
};

class NamespaceExpression final : public DebuggableExpression
{
public:
	NamespaceExpression(std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Expression(std::move(expression))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::shared_ptr<Expression> m_Expression;
};

class ObjectExpression final : public DebuggableExpression
{
public:
	ObjectExpression(bool abstract, std::unique_ptr<Expression> type, std::unique_ptr<Expression> name, std::unique_ptr<Expression> filter,
		String zone, String package, std::map<String, std::unique_ptr<Expression> >&& closedVars,
		bool defaultTmpl, bool ignoreOnError, std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Abstract(abstract), m_Type(std::move(type)),
		m_Name(std::move(name)), m_Filter(std::move(filter)), m_Zone(std::move(zone)), m_Package(std::move(package)), m_DefaultTmpl(defaultTmpl),
		m_IgnoreOnError(ignoreOnError), m_ClosedVars(std::move(closedVars)), m_Expression(std::move(expression))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	bool m_Abstract;
	std::unique_ptr<Expression> m_Type;
	std::unique_ptr<Expression> m_Name;
	std::shared_ptr<Expression> m_Filter;
	String m_Zone;
	String m_Package;
	bool m_DefaultTmpl;
	bool m_IgnoreOnError;
	std::map<String, std::unique_ptr<Expression> > m_ClosedVars;
	std::shared_ptr<Expression> m_Expression;
};

class ForExpression final : public DebuggableExpression
{
public:
	ForExpression(String fkvar, String fvvar, std::unique_ptr<Expression> value, std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_FKVar(std::move(fkvar)), m_FVVar(std::move(fvvar)), m_Value(std::move(value)), m_Expression(std::move(expression))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_FKVar;
	String m_FVVar;
	std::unique_ptr<Expression> m_Value;
	std::unique_ptr<Expression> m_Expression;
};

class LibraryExpression final : public UnaryExpression
{
public:
	LibraryExpression(std::unique_ptr<Expression> expression, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(std::move(expression), debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

enum IncludeType
{
	IncludeRegular,
	IncludeRecursive,
	IncludeZones
};

class IncludeExpression final : public DebuggableExpression
{
public:
	IncludeExpression(String relativeBase, std::unique_ptr<Expression> path, std::unique_ptr<Expression> pattern, std::unique_ptr<Expression> name,
		IncludeType type, bool searchIncludes, String zone, String package, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_RelativeBase(std::move(relativeBase)), m_Path(std::move(path)), m_Pattern(std::move(pattern)),
		m_Name(std::move(name)), m_Type(type), m_SearchIncludes(searchIncludes), m_Zone(std::move(zone)), m_Package(std::move(package))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_RelativeBase;
	std::unique_ptr<Expression> m_Path;
	std::unique_ptr<Expression> m_Pattern;
	std::unique_ptr<Expression> m_Name;
	IncludeType m_Type;
	bool m_SearchIncludes;
	String m_Zone;
	String m_Package;
};

class BreakpointExpression final : public DebuggableExpression
{
public:
	BreakpointExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class TryExceptExpression final : public DebuggableExpression
{
public:
	TryExceptExpression(std::unique_ptr<Expression> tryBody, std::unique_ptr<Expression> exceptBody, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_TryBody(std::move(tryBody)), m_ExceptBody(std::move(exceptBody))
	{ }

protected:
	ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::unique_ptr<Expression> m_TryBody;
	std::unique_ptr<Expression> m_ExceptBody;
};

}

#endif /* EXPRESSION_H */
