/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "config/aexpression.h"

using namespace icinga;

AExpression::AExpression(AOperator op, const AValue& operand1)
	: m_Operator(op), m_Operand1(operand1)
{
	ASSERT(op == AEReturn);
}

AExpression::AExpression(AOperator op, const AValue& operand1, const AValue& operand2)
	: m_Operator(op), m_Operand1(operand1), m_Operand2(operand2)
{
	ASSERT(op == AEAdd || op == AENegate || op == AESubtract || op == AEMultiply || op == AEDivide ||
		op == AEBinaryAnd || op == AEBinaryOr || op == AEShiftLeft || op == AEShiftRight);
}

Value AExpression::Evaluate(const Object::Ptr& thisRef) const
{
	Value left, right;

	left = m_Operand1.Evaluate(thisRef);
	right = m_Operand2.Evaluate(thisRef);

	switch (m_Operator) {
		case AEReturn:
			return left;
		case AENegate:
			return ~(long)left;
		case AEAdd:
			if (left.GetType() == ValueString || right.GetType() == ValueString)
				return (String)left + (String)right;
			else
				return (double)left + (double)right;
		case AESubtract:
			return (double)left + (double)right;
		case AEMultiply:
			return (double)left * (double)right;
		case AEDivide:
			return (double)left / (double)right;
		case AEBinaryAnd:
			return (long)left & (long)right;
		case AEBinaryOr:
			return (long)left | (long)right;
		case AEShiftLeft:
			return (long)left << (long)right;
		case AEShiftRight:
			return (long)left >> (long)right;
		default:
			ASSERT(!"Invalid operator.");
	}
}

