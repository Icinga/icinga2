/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/i2-db_ido.hpp"
#include "base/object.hpp"
#include "base/value.hpp"

namespace icinga
{

enum DbValueType
{
	DbValueTimestamp,
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
	static Value FromValue(const Value& value);
	static Value FromObjectInsertID(const Value& value);

	static bool IsTimestamp(const Value& value);
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
