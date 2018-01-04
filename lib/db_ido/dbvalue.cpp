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

#include "db_ido/dbvalue.hpp"

using namespace icinga;

DbValue::DbValue(DbValueType type, const Value& value)
	: m_Type(type), m_Value(value)
{ }

Value DbValue::FromTimestamp(const Value& ts)
{
	if (ts.IsEmpty() || ts == 0)
		return Empty;

	return new DbValue(DbValueTimestamp, ts);
}

Value DbValue::FromTimestampNow()
{
	return new DbValue(DbValueTimestampNow, Empty);
}

Value DbValue::FromValue(const Value& value)
{
	return value;
}

Value DbValue::FromObjectInsertID(const Value& value)
{
	return new DbValue(DbValueObjectInsertID, value);
}

bool DbValue::IsTimestamp(const Value& value)
{
	if (!value.IsObjectType<DbValue>())
		return false;

	DbValue::Ptr dbv = value;
	return dbv->GetType() == DbValueTimestamp;
}

bool DbValue::IsTimestampNow(const Value& value)
{
	if (!value.IsObjectType<DbValue>())
		return false;

	DbValue::Ptr dbv = value;
	return dbv->GetType() == DbValueTimestampNow;
}

bool DbValue::IsObjectInsertID(const Value& value)
{
	if (!value.IsObjectType<DbValue>())
		return false;

	DbValue::Ptr dbv = value;
	return dbv->GetType() == DbValueObjectInsertID;
}

Value DbValue::ExtractValue(const Value& value)
{
	if (!value.IsObjectType<DbValue>())
		return value;

	DbValue::Ptr dbv = value;
	return dbv->GetValue();
}

DbValueType DbValue::GetType() const
{
	return m_Type;
}

Value DbValue::GetValue() const
{
	return m_Value;
}

void DbValue::SetValue(const Value& value)
{
	m_Value = value;
}
