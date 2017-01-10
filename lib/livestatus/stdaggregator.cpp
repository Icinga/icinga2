/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "livestatus/stdaggregator.hpp"
#include <math.h>

using namespace icinga;

StdAggregator::StdAggregator(const String& attr)
    : m_StdSum(0), m_StdQSum(0), m_StdCount(0), m_StdAttr(attr)
{ }

void StdAggregator::Apply(const Table::Ptr& table, const Value& row)
{
	Column column = table->GetColumn(m_StdAttr);

	Value value = column.ExtractValue(row);

	m_StdSum += value;
	m_StdQSum += pow(value, 2);
	m_StdCount++;
}

double StdAggregator::GetResult(void) const
{
	return sqrt((m_StdQSum - (1 / m_StdCount) * pow(m_StdSum, 2)) / (m_StdCount - 1));
}
