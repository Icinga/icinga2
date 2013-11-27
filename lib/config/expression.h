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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "config/i2-config.h"
#include "config/debuginfo.h"
#include "base/dictionary.h"
#include <iostream>
#include <vector>
#include <set>

namespace icinga
{

/**
 * The operator in a configuration expression.
 *
 * @ingroup config
 */
enum ExpressionOperator
{
	OperatorNop,
	OperatorExecute,
	OperatorSet,
	OperatorPlus,
	OperatorMinus,
	OperatorMultiply,
	OperatorDivide
};

class ExpressionList;

/**
 * A configuration expression.
 *
 * @ingroup config
 */
struct I2_CONFIG_API Expression
{
public:
	Expression(const String& key, ExpressionOperator op, const Value& value,
	    const DebugInfo& debuginfo);

	void Execute(const Dictionary::Ptr& dictionary) const;

	void ExtractPath(const std::vector<String>& path, const shared_ptr<ExpressionList>& result) const;
	void ExtractFiltered(const std::set<String>& keys, const shared_ptr<ExpressionList>& result) const;

	void ErasePath(const std::vector<String>& path);

	void FindDebugInfoPath(const std::vector<String>& path, DebugInfo& result) const;

private:
	String m_Key;
	ExpressionOperator m_Operator;
	Value m_Value;
	DebugInfo m_DebugInfo;

	static Value DeepClone(const Value& value);
};

}

#endif /* EXPRESSION_H */
