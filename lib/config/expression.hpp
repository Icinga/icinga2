/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
	DebugHint(const Dictionary::Ptr& hints = nullptr)
		: m_Hints(hints)
	{ }

	DebugHint(Dictionary::Ptr&& hints)
	    : m_Hints(std::move(hints))
	{ }

	inline void AddMessage(const String& message, const DebugInfo& di)
	{
		GetMessages()->Add(new Array({ message, di.Path, di.FirstLine, di.FirstColumn, di.LastLine, di.LastColumn }));
	}

	inline DebugHint GetChild(const String& name)
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

	Dictionary::Ptr ToDictionary(void) const
	{
		return m_Hints;
	}

private:
	Dictionary::Ptr m_Hints;
	Array::Ptr m_Messages;
	Dictionary::Ptr m_Children;

	const Array::Ptr& GetMessages(void)
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

	const Dictionary::Ptr& GetChildren(void)
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
	ExpressionResult(const T& value, ExpressionResultCode code = ResultOK)
	    : m_Value(value), m_Code(code)
	{ }

	operator const Value&(void) const
	{
		return m_Value;
	}

	const Value& GetValue(void) const
	{
		return m_Value;
	}

	ExpressionResultCode GetCode(void) const
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
class I2_CONFIG_API Expression
{
public:
	virtual ~Expression(void);

	ExpressionResult Evaluate(ScriptFrame& frame, DebugHint *dhint = NULL) const;
	virtual bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint = NULL) const;
	virtual const DebugInfo& GetDebugInfo(void) const;

	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const = 0;

	static boost::signals2::signal<void (ScriptFrame& frame, ScriptError *ex, const DebugInfo& di)> OnBreakpoint;

	static void ScriptBreakpoint(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di);
};

I2_CONFIG_API Expression *MakeIndexer(ScopeSpecifier scopeSpec, const String& index);

class I2_CONFIG_API OwnedExpression : public Expression
{
public:
	OwnedExpression(const std::shared_ptr<Expression>& expression)
		: m_Expression(expression)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override
	{
		return m_Expression->DoEvaluate(frame, dhint);
	}

	virtual const DebugInfo& GetDebugInfo(void) const override
	{
		return m_Expression->GetDebugInfo();
	}

private:
	std::shared_ptr<Expression> m_Expression;
};

class I2_CONFIG_API LiteralExpression : public Expression
{
public:
	LiteralExpression(const Value& value = Value());

	const Value& GetValue(void) const
	{
		return m_Value;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Value m_Value;
};

inline LiteralExpression *MakeLiteral(const Value& literal = Value())
{
	return new LiteralExpression(literal);
}

class I2_CONFIG_API DebuggableExpression : public Expression
{
public:
	DebuggableExpression(const DebugInfo& debugInfo = DebugInfo())
		: m_DebugInfo(debugInfo)
	{ }

protected:
	virtual const DebugInfo& GetDebugInfo(void) const override;

	DebugInfo m_DebugInfo;
};

class I2_CONFIG_API UnaryExpression : public DebuggableExpression
{
public:
	UnaryExpression(Expression *operand, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Operand(operand)
	{ }

	~UnaryExpression(void)
	{
		delete m_Operand;
	}

protected:
	Expression *m_Operand;
};

class I2_CONFIG_API BinaryExpression : public DebuggableExpression
{
public:
	BinaryExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Operand1(operand1), m_Operand2(operand2)
	{ }
	
	~BinaryExpression(void)
	{
		delete m_Operand1;
		delete m_Operand2;
	}

protected:
	Expression *m_Operand1;
	Expression *m_Operand2;
};

class I2_CONFIG_API VariableExpression : public DebuggableExpression
{
public:
	VariableExpression(const String& variable, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Variable(variable)
	{ }

	String GetVariable(void) const
	{
		return m_Variable;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
	virtual bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const override;

private:
	String m_Variable;

	friend I2_CONFIG_API void BindToScope(Expression *& expr, ScopeSpecifier scopeSpec);
};
	
class I2_CONFIG_API NegateExpression : public UnaryExpression
{
public:
	NegateExpression(Expression *operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(operand, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API LogicalNegateExpression : public UnaryExpression
{
public:
	LogicalNegateExpression(Expression *operand, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(operand, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API AddExpression : public BinaryExpression
{
public:
	AddExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API SubtractExpression : public BinaryExpression
{
public:
	SubtractExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API MultiplyExpression : public BinaryExpression
{
public:
	MultiplyExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API DivideExpression : public BinaryExpression
{
public:
	DivideExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API ModuloExpression : public BinaryExpression
{
public:
	ModuloExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API XorExpression : public BinaryExpression
{
public:
	XorExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API BinaryAndExpression : public BinaryExpression
{
public:
	BinaryAndExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API BinaryOrExpression : public BinaryExpression
{
public:
	BinaryOrExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API ShiftLeftExpression : public BinaryExpression
{
public:
	ShiftLeftExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API ShiftRightExpression : public BinaryExpression
{
public:
	ShiftRightExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API EqualExpression : public BinaryExpression
{
public:
	EqualExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API NotEqualExpression : public BinaryExpression
{
public:
	NotEqualExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API LessThanExpression : public BinaryExpression
{
public:
	LessThanExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API GreaterThanExpression : public BinaryExpression
{
public:
	GreaterThanExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API LessThanOrEqualExpression : public BinaryExpression
{
public:
	LessThanOrEqualExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API GreaterThanOrEqualExpression : public BinaryExpression
{
public:
	GreaterThanOrEqualExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API InExpression : public BinaryExpression
{
public:
	InExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API NotInExpression : public BinaryExpression
{
public:
	NotInExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API LogicalAndExpression : public BinaryExpression
{
public:
	LogicalAndExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API LogicalOrExpression : public BinaryExpression
{
public:
	LogicalOrExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};
	
class I2_CONFIG_API FunctionCallExpression : public DebuggableExpression
{
public:
	FunctionCallExpression(Expression *fname, const std::vector<Expression *>& args, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_FName(fname), m_Args(args)
	{ }

	~FunctionCallExpression(void)
	{
		delete m_FName;

		for (Expression *expr : m_Args)
			delete expr;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

public:
	Expression *m_FName;
	std::vector<Expression *> m_Args;
};
	
class I2_CONFIG_API ArrayExpression : public DebuggableExpression
{
public:
	ArrayExpression(const std::vector<Expression *>& expressions, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Expressions(expressions)
	{ }

	~ArrayExpression(void)
	{
		for (Expression *expr : m_Expressions)
			delete expr;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::vector<Expression *> m_Expressions;
};
	
class I2_CONFIG_API DictExpression : public DebuggableExpression
{
public:
	DictExpression(const std::vector<Expression *>& expressions = std::vector<Expression *>(), const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Expressions(expressions), m_Inline(false)
	{ }

	~DictExpression(void)
	{
		for (Expression *expr : m_Expressions)
			delete expr;
	}

	void MakeInline(void);

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	std::vector<Expression *> m_Expressions;
	bool m_Inline;

	friend I2_CONFIG_API void BindToScope(Expression *& expr, ScopeSpecifier scopeSpec);
};
	
class I2_CONFIG_API SetExpression : public BinaryExpression
{
public:
	SetExpression(Expression *operand1, CombinedSetOp op, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo), m_Op(op)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	CombinedSetOp m_Op;

	friend I2_CONFIG_API void BindToScope(Expression *& expr, ScopeSpecifier scopeSpec);
};

class I2_CONFIG_API ConditionalExpression : public DebuggableExpression
{
public:
	ConditionalExpression(Expression *condition, Expression *true_branch, Expression *false_branch, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Condition(condition), m_TrueBranch(true_branch), m_FalseBranch(false_branch)
	{ }

	~ConditionalExpression(void)
	{
		delete m_Condition;
		delete m_TrueBranch;
		delete m_FalseBranch; 
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Expression *m_Condition;
	Expression *m_TrueBranch;
	Expression *m_FalseBranch;
};

class I2_CONFIG_API WhileExpression : public DebuggableExpression
{
public:
	WhileExpression(Expression *condition, Expression *loop_body, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Condition(condition), m_LoopBody(loop_body)
	{ }

	~WhileExpression(void)
	{
		delete m_Condition;
		delete m_LoopBody;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Expression *m_Condition;
	Expression *m_LoopBody;
};


class I2_CONFIG_API ReturnExpression : public UnaryExpression
{
public:
	ReturnExpression(Expression *expression, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(expression, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API BreakExpression : public DebuggableExpression
{
public:
	BreakExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API ContinueExpression : public DebuggableExpression
{
public:
	ContinueExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API GetScopeExpression : public Expression
{
public:
	GetScopeExpression(ScopeSpecifier scopeSpec)
		: m_ScopeSpec(scopeSpec)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	ScopeSpecifier m_ScopeSpec;
};

class I2_CONFIG_API IndexerExpression : public BinaryExpression
{
public:
	IndexerExpression(Expression *operand1, Expression *operand2, const DebugInfo& debugInfo = DebugInfo())
		: BinaryExpression(operand1, operand2, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
	virtual bool GetReference(ScriptFrame& frame, bool init_dict, Value *parent, String *index, DebugHint **dhint) const override;

	friend I2_CONFIG_API void BindToScope(Expression *& expr, ScopeSpecifier scopeSpec);
};

I2_CONFIG_API void BindToScope(Expression *& expr, ScopeSpecifier scopeSpec);

class I2_CONFIG_API ThrowExpression : public DebuggableExpression
{
public:
	ThrowExpression(Expression *message, bool incompleteExpr, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Message(message), m_IncompleteExpr(incompleteExpr)
	{ }

	~ThrowExpression(void)
	{
		delete m_Message;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Expression *m_Message;
	bool m_IncompleteExpr;
};

class I2_CONFIG_API ImportExpression : public DebuggableExpression
{
public:
	ImportExpression(Expression *name, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Name(name)
	{ }

	~ImportExpression(void)
	{
		delete m_Name;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Expression *m_Name;
};

class I2_CONFIG_API ImportDefaultTemplatesExpression : public DebuggableExpression
{
public:
	ImportDefaultTemplatesExpression(const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API FunctionExpression : public DebuggableExpression
{
public:
	FunctionExpression(const String& name, const std::vector<String>& args,
	    std::map<String, Expression *> *closedVars, Expression *expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Name(name), m_Args(args), m_ClosedVars(closedVars), m_Expression(expression)
	{ }

	~FunctionExpression(void)
	{
		if (m_ClosedVars) {
			typedef std::pair<String, Expression *> kv_pair;
			for (const kv_pair& kv : *m_ClosedVars) {
				delete kv.second;
			}
		}

		delete m_ClosedVars;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_Name;
	std::vector<String> m_Args;
	std::map<String, Expression *> *m_ClosedVars;
	std::shared_ptr<Expression> m_Expression;
};

class I2_CONFIG_API ApplyExpression : public DebuggableExpression
{
public:
	ApplyExpression(const String& type, const String& target, Expression *name,
	    Expression *filter, const String& package, const String& fkvar, const String& fvvar,
	    Expression *fterm, std::map<String, Expression *> *closedVars, bool ignoreOnError,
	    Expression *expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Type(type), m_Target(target),
		    m_Name(name), m_Filter(filter), m_Package(package), m_FKVar(fkvar), m_FVVar(fvvar),
		    m_FTerm(fterm), m_IgnoreOnError(ignoreOnError), m_ClosedVars(closedVars),
		    m_Expression(expression)
	{ }

	~ApplyExpression(void)
	{
		delete m_Name;

		if (m_ClosedVars) {
			typedef std::pair<String, Expression *> kv_pair;
			for (const kv_pair& kv : *m_ClosedVars) {
				delete kv.second;
			}
		}

		delete m_ClosedVars;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_Type;
	String m_Target;
	Expression *m_Name;
	std::shared_ptr<Expression> m_Filter;
	String m_Package;
	String m_FKVar;
	String m_FVVar;
	std::shared_ptr<Expression> m_FTerm;
	bool m_IgnoreOnError;
	std::map<String, Expression *> *m_ClosedVars;
	std::shared_ptr<Expression> m_Expression;
};

class I2_CONFIG_API ObjectExpression : public DebuggableExpression
{
public:
	ObjectExpression(bool abstract, Expression *type, Expression *name, Expression *filter,
	    const String& zone, const String& package, std::map<String, Expression *> *closedVars,
	    bool defaultTmpl, bool ignoreOnError, Expression *expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Abstract(abstract), m_Type(type),
		  m_Name(name), m_Filter(filter), m_Zone(zone), m_Package(package), m_DefaultTmpl(defaultTmpl),
		  m_IgnoreOnError(ignoreOnError), m_ClosedVars(closedVars), m_Expression(expression)
	{ }

	~ObjectExpression(void)
	{
		delete m_Name;

		if (m_ClosedVars) {
			typedef std::pair<String, Expression *> kv_pair;
			for (const kv_pair& kv : *m_ClosedVars) {
				delete kv.second;
			}
		}

		delete m_ClosedVars;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	bool m_Abstract;
	Expression *m_Type;
	Expression *m_Name;
	std::shared_ptr<Expression> m_Filter;
	String m_Zone;
	String m_Package;
	bool m_DefaultTmpl;
	bool m_IgnoreOnError;
	std::map<String, Expression *> *m_ClosedVars;
	std::shared_ptr<Expression> m_Expression;
};
	
class I2_CONFIG_API ForExpression : public DebuggableExpression
{
public:
	ForExpression(const String& fkvar, const String& fvvar, Expression *value, Expression *expression, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_FKVar(fkvar), m_FVVar(fvvar), m_Value(value), m_Expression(expression)
	{ }

	~ForExpression(void)
	{
		delete m_Value;
		delete m_Expression;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_FKVar;
	String m_FVVar;
	Expression *m_Value;
	Expression *m_Expression;
};

class I2_CONFIG_API LibraryExpression : public UnaryExpression
{
public:
	LibraryExpression(Expression *expression, const DebugInfo& debugInfo = DebugInfo())
		: UnaryExpression(expression, debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

enum IncludeType
{
	IncludeRegular,
	IncludeRecursive,
	IncludeZones
};

class I2_CONFIG_API IncludeExpression : public DebuggableExpression
{
public:
	IncludeExpression(const String& relativeBase, Expression *path, Expression *pattern, Expression *name,
	    IncludeType type, bool searchIncludes, const String& zone, const String& package, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_RelativeBase(relativeBase), m_Path(path), m_Pattern(pattern),
		  m_Name(name), m_Type(type), m_SearchIncludes(searchIncludes), m_Zone(zone), m_Package(package)
	{ }

	~IncludeExpression(void)
	{
		delete m_Path;
		delete m_Pattern;
		delete m_Name;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	String m_RelativeBase;
	Expression *m_Path;
	Expression *m_Pattern;
	Expression *m_Name;
	IncludeType m_Type;
	bool m_SearchIncludes;
	String m_Zone;
	String m_Package;
};

class I2_CONFIG_API BreakpointExpression : public DebuggableExpression
{
public:
	BreakpointExpression(const DebugInfo& debugInfo = DebugInfo())
	    : DebuggableExpression(debugInfo)
	{ }

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;
};

class I2_CONFIG_API UsingExpression : public DebuggableExpression
{
public:
	UsingExpression(Expression *name, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_Name(name)
	{ }

	~UsingExpression(void)
	{
		delete m_Name;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Expression *m_Name;
};

class I2_CONFIG_API TryExceptExpression : public DebuggableExpression
{
public:
	TryExceptExpression(Expression *tryBody, Expression *exceptBody, const DebugInfo& debugInfo = DebugInfo())
		: DebuggableExpression(debugInfo), m_TryBody(tryBody), m_ExceptBody(exceptBody)
	{ }

	~TryExceptExpression(void)
	{
		delete m_TryBody;
		delete m_ExceptBody;
	}

protected:
	virtual ExpressionResult DoEvaluate(ScriptFrame& frame, DebugHint *dhint) const override;

private:
	Expression *m_TryBody;
	Expression *m_ExceptBody;
};

}

#endif /* EXPRESSION_H */
