/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef DBVALUE_H
#define DBVALUE_H

#include "db_ido/i2-db_ido.hpp"
#include "base/object.hpp"
#include "base/value.hpp"

namespace icinga
{

enum DbValueType
{
	DbValueTimestamp,
	DbValueTimestampNow,
	DbValueObjectInsertID
};

/**
 * A database value.
 *
 * @ingroup ido
 */
struct DbValue final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbValue);

	DbValue(DbValueType type, Value value);

	static Value FromTimestamp(const Value& ts);
	static Value FromTimestampNow();
	static Value FromValue(const Value& value);
	static Value FromObjectInsertID(const Value& value);

	static bool IsTimestamp(const Value& value);
	static bool IsTimestampNow(const Value& value);
	static bool IsObjectInsertID(const Value& value);

	static Value ExtractValue(const Value& value);

	DbValueType GetType() const;

	Value GetValue() const;
	void SetValue(const Value& value);

private:
	DbValueType m_Type;
	Value m_Value;
};

}

#endif /* DBVALUE_H */
