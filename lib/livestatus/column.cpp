// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
