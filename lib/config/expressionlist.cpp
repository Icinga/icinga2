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

#include "config/expressionlist.h"
#include <boost/foreach.hpp>

using namespace icinga;

/**
 * Adds an expression to an expression list.
 *
 * @param expression The expression that should be added.
 */
void ExpressionList::AddExpression(const Expression& expression)
{
	m_Expressions.push_back(expression);
}

/**
 * Returns the number of items currently contained in the expression list.
 *
 * @returns The length of the list.
 */
size_t ExpressionList::GetLength(void) const
{
	return m_Expressions.size();
}

/**
 * Executes the expression list.
 *
 * @param dictionary The dictionary that should be manipulated by the
 *		     expressions.
 */
void ExpressionList::Execute(const Dictionary::Ptr& dictionary) const
{
	BOOST_FOREACH(const Expression& expression, m_Expressions) {
		expression.Execute(dictionary);
	}
}

void ExpressionList::ExtractPath(const std::vector<String>& path, const ExpressionList::Ptr& result) const
{
	BOOST_FOREACH(const Expression& expression, m_Expressions) {
		expression.ExtractPath(path, result);
	}
}

void ExpressionList::ExtractFiltered(const std::set<String>& keys, const ExpressionList::Ptr& result) const
{
	BOOST_FOREACH(const Expression& expression, m_Expressions) {
		expression.ExtractFiltered(keys, result);
	}
}

void ExpressionList::ErasePath(const std::vector<String>& path)
{
	BOOST_FOREACH(Expression& expression, m_Expressions) {
		expression.ErasePath(path);
	}
}

void ExpressionList::FindDebugInfoPath(const std::vector<String>& path, DebugInfo& result) const
{
	BOOST_FOREACH(const Expression& expression, m_Expressions) {
		expression.FindDebugInfoPath(path, result);
	}
}
