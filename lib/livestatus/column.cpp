/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/column.hpp"

using namespace icinga;

Column::Column(ValueAccessor valueAccessor, ObjectAccessor objectAccessor)
	: m_ValueAccessor(std::move(valueAccessor)), m_ObjectAccessor(std::move(objectAccessor))
{ }

Value Column::ExtractValue(const Value& urow, LivestatusGroupByType groupByType, const Object::Ptr& groupByObject) const
{
	Value row;

	if (m_ObjectAccessor)
		row = m_ObjectAccessor(urow, groupByType, groupByObject);
	else
		row = urow;

	return m_ValueAccessor(row);
}
