/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef EXPRESSIONLIST_H
#define EXPRESSIONLIST_H

namespace icinga
{

class I2_DYN_API ExpressionList : public Object
{
public:
	typedef shared_ptr<ExpressionList> Ptr;
	typedef weak_ptr<ExpressionList> WeakPtr;

	ExpressionList(void);
//	ExpressionList(Dictionary::Ptr serializedDictionary);

	void AddExpression(const Expression& expression);

	void Execute(const Dictionary::Ptr& dictionary) const;

	size_t GetLength(void) const;

//	Dictionary::Ptr Serialize(void);

private:
	vector<Expression> m_Expressions;
};

}

#endif /* EXPRESSIONLIST_H */
