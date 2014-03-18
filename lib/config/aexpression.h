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
#include "config/avalue.h"
#include "config/debuginfo.h"
#include "base/dictionary.h"

namespace icinga
{

/**
 * @ingroup config
 */
enum AOperator
{
	AEReturn,
	AENegate,
	AEAdd,
	AESubtract,
	AEMultiply,
	AEDivide,
	AEBinaryAnd,
	AEBinaryOr,
	AEShiftLeft,
	AEShiftRight,
	AEEqual,
	AENotEqual,
	AEIn,
	AENotIn,
	AELogicalAnd,
	AELogicalOr
};

/**
 * @ingroup config
 */
class I2_CONFIG_API AExpression : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(AExpression);

	AExpression(AOperator op, const AValue& operand1, const DebugInfo& di);
	AExpression(AOperator op, const AValue& operand1, const AValue& operand2, const DebugInfo& di);

	Value Evaluate(const Dictionary::Ptr& locals) const;

private:
	AOperator m_Operator;
	AValue m_Operand1;
	AValue m_Operand2;
	DebugInfo m_DebugInfo;
};

}

#endif /* TYPERULE_H */
