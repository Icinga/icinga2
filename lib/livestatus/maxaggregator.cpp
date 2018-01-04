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

#include "livestatus/maxaggregator.hpp"

using namespace icinga;

MaxAggregator::MaxAggregator(String attr)
	: m_MaxAttr(std::move(attr))
{ }

MaxAggregatorState *MaxAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new MaxAggregatorState();

	return static_cast<MaxAggregatorState *>(*state);
}

void MaxAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_MaxAttr);

	Value value = column.ExtractValue(row);

	MaxAggregatorState *pstate = EnsureState(state);

	if (value > pstate->Max)
		pstate->Max = value;
}

double MaxAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	MaxAggregatorState *pstate = EnsureState(&state);
	double result = pstate->Max;
	delete pstate;

	return result;
}
