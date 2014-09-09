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

#ifndef AEXPRESSION_H
#define AEXPRESSION_H

#include "config/i2-config.hpp"
#include "base/debuginfo.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

struct DebugHint
{
	std::vector<std::pair<String, DebugInfo> > Messages;
	std::map<String, DebugHint> Children;

	inline void AddMessage(const String& message, const DebugInfo& di)
	{
		Messages.push_back(std::make_pair(message, di));
	}

	inline DebugHint *GetChild(const String& name)
	{
		return &Children[name];
	}

	Dictionary::Ptr ToDictionary(void) const;
};

/**
 * @ingroup config
 */
class I2_CONFIG_API AExpression : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(AExpression);
	
	typedef Value (*OpCallback)(const AExpression *, const Dictionary::Ptr&, DebugHint *dhint);

	AExpression(OpCallback op, const Value& operand1, const DebugInfo& di);
	AExpression(OpCallback op, const Value& operand1, const Value& operand2, const DebugInfo& di);

	Value Evaluate(const Dictionary::Ptr& locals, DebugHint *dhint = NULL) const;

	void MakeInline(void);
	
	void Dump(std::ostream& stream, int indent = 0) const;

	static Value OpLiteral(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpVariable(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpNegate(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpLogicalNegate(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpAdd(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpSubtract(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpMultiply(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpDivide(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpBinaryAnd(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpBinaryOr(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpShiftLeft(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpShiftRight(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpEqual(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpNotEqual(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpLessThan(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpGreaterThan(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpLessThanOrEqual(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpGreaterThanOrEqual(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpIn(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpNotIn(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpLogicalAnd(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpLogicalOr(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpFunctionCall(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpArray(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpDict(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpSet(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpSetPlus(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpSetMinus(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpSetMultiply(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpSetDivide(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpIndexer(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpImport(const AExpression *expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpFunction(const AExpression* expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpApply(const AExpression* expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpObject(const AExpression* expr, const Dictionary::Ptr& locals, DebugHint *dhint);
	static Value OpFor(const AExpression* expr, const Dictionary::Ptr& locals, DebugHint *dhint);

private:
	OpCallback m_Operator;
	Value m_Operand1;
	Value m_Operand2;
	DebugInfo m_DebugInfo;

	Value EvaluateOperand1(const Dictionary::Ptr& locals, DebugHint *dhint = NULL) const;
	Value EvaluateOperand2(const Dictionary::Ptr& locals, DebugHint *dhint = NULL) const;

	static void DumpOperand(std::ostream& stream, const Value& operand, int indent);

	static Value FunctionWrapper(const std::vector<Value>& arguments, const Array::Ptr& funcargs,
	    const AExpression::Ptr& expr, const Dictionary::Ptr& scope);
};

}

#endif /* TYPERULE_H */
