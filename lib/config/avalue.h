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

#ifndef AVALUE_H
#define AVALUE_H

#include "config/i2-config.h"
#include "base/value.h"
#include "base/object.h"

namespace icinga
{

/**
 * @ingroup config
 */
enum AValueType
{
	ATSimple,
	ATVariable,
	ATThisRef,
	ATExpression
};

class AExpression;

/**
 * @ingroup config
 */
class I2_CONFIG_API AValue
{
public:
	AValue(void);
	AValue(const shared_ptr<AExpression>& expr);
	AValue(AValueType type, const Value& value);

	Value Evaluate(const Object::Ptr& thisRef) const;

private:
	AValueType m_Type;
	Value m_Value;
	shared_ptr<AExpression> m_Expression;
};

}

#endif /* AVALUE_H */