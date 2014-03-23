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

#include "config/i2-config.h"
#include "config/debuginfo.h"
#include "base/array.h"
#include "base/dictionary.h"

namespace icinga
{

/**
 * @ingroup config
 */
class I2_CONFIG_API AExpression : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(AExpression);
	
	typedef Value (AExpression::*OpCallback)(const Dictionary::Ptr&) const;

	AExpression(OpCallback op, const Value& operand1, const DebugInfo& di);
	AExpression(OpCallback op, const Value& operand1, const Value& operand2, const DebugInfo& di);

	Value Evaluate(const Dictionary::Ptr& locals) const;
	void ExtractPath(const std::vector<String>& path, const Array::Ptr& result) const;
	void FindDebugInfoPath(const std::vector<String>& path, DebugInfo& result) const;

	void MakeInline(void);
	
	Value OpLiteral(const Dictionary::Ptr& locals) const;
	Value OpVariable(const Dictionary::Ptr& locals) const;
	Value OpNegate(const Dictionary::Ptr& locals) const;
	Value OpAdd(const Dictionary::Ptr& locals) const;
	Value OpSubtract(const Dictionary::Ptr& locals) const;
	Value OpMultiply(const Dictionary::Ptr& locals) const;
	Value OpDivide(const Dictionary::Ptr& locals) const;
	Value OpBinaryAnd(const Dictionary::Ptr& locals) const;
	Value OpBinaryOr(const Dictionary::Ptr& locals) const;
	Value OpShiftLeft(const Dictionary::Ptr& locals) const;
	Value OpShiftRight(const Dictionary::Ptr& locals) const;
	Value OpEqual(const Dictionary::Ptr& locals) const;
	Value OpNotEqual(const Dictionary::Ptr& locals) const;
	Value OpLessThan(const Dictionary::Ptr& locals) const;
	Value OpGreaterThan(const Dictionary::Ptr& locals) const;
	Value OpLessThanOrEqual(const Dictionary::Ptr& locals) const;
	Value OpGreaterThanOrEqual(const Dictionary::Ptr& locals) const;
	Value OpIn(const Dictionary::Ptr& locals) const;
	Value OpNotIn(const Dictionary::Ptr& locals) const;
	Value OpLogicalAnd(const Dictionary::Ptr& locals) const;
	Value OpLogicalOr(const Dictionary::Ptr& locals) const;
	Value OpFunctionCall(const Dictionary::Ptr& locals) const;
	Value OpArray(const Dictionary::Ptr& locals) const;
	Value OpDict(const Dictionary::Ptr& locals) const;
	Value OpSet(const Dictionary::Ptr& locals) const;
	Value OpSetPlus(const Dictionary::Ptr& locals) const;
	Value OpSetMinus(const Dictionary::Ptr& locals) const;
	Value OpSetMultiply(const Dictionary::Ptr& locals) const;
	Value OpSetDivide(const Dictionary::Ptr& locals) const;

private:
	OpCallback m_Operator;
	Value m_Operand1;
	Value m_Operand2;
	DebugInfo m_DebugInfo;

	Value EvaluateOperand1(const Dictionary::Ptr& locals) const;
	Value EvaluateOperand2(const Dictionary::Ptr& locals) const;
};

}

#endif /* TYPERULE_H */
