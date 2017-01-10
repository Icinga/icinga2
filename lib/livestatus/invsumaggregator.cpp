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

#include "livestatus/invsumaggregator.hpp"

using namespace icinga;

InvSumAggregator::InvSumAggregator(const String& attr)
    : m_InvSum(0), m_InvSumAttr(attr)
{ }

void InvSumAggregator::Apply(const Table::Ptr& table, const Value& row)
{
	Column column = table->GetColumn(m_InvSumAttr);

	Value value = column.ExtractValue(row);

	m_InvSum += (1.0 / value);
}

double InvSumAggregator::GetResult(void) const
{
	return m_InvSum;
}
