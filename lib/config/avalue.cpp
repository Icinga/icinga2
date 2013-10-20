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

#include "config/avalue.h"
#include "config/aexpression.h"
#include "base/scriptvariable.h"

using namespace icinga;

AValue::AValue(void)
	: m_Type(ATSimple)
{ }

AValue::AValue(const AExpression::Ptr& expr)
	: m_Type(ATExpression), m_Expression(expr)
{ }

AValue::AValue(AValueType type, const Value& value)
	: m_Type(type), m_Value(value)
{ }

Value AValue::Evaluate(const Object::Ptr& thisRef) const
{
	switch (m_Type) {
		case ATSimple:
			return m_Value;
		case ATVariable:
			return ScriptVariable::Get(m_Value);
		case ATThisRef:
			VERIFY(!"Not implemented.");
		case ATExpression:
			return m_Expression->Evaluate(thisRef);
		default:
			ASSERT(!"Invalid type.");
	}
}
