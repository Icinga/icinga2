/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/dbvalue.hpp"

using namespace icinga;

DbValue::DbValue(DbValueType type, Value value)
	: m_Type(type), m_Value(std::move(value))
{ }

Value DbValue::FromTimestamp(const Value& ts)
{
	if (ts.IsEmpty() || ts == 0)
		return Empty;

	return new DbValue(DbValueTimestamp, ts);
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
