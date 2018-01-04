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

#include "livestatus/invavgaggregator.hpp"

using namespace icinga;

InvAvgAggregator::InvAvgAggregator(String attr)
	: m_InvAvgAttr(std::move(attr))
{ }

InvAvgAggregatorState *InvAvgAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new InvAvgAggregatorState();

	return static_cast<InvAvgAggregatorState *>(*state);
}

void InvAvgAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_InvAvgAttr);

	Value value = column.ExtractValue(row);

	InvAvgAggregatorState *pstate = EnsureState(state);

	pstate->InvAvg += (1.0 / value);
	pstate->InvAvgCount++;
}

double InvAvgAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	InvAvgAggregatorState *pstate = EnsureState(&state);
	double result = pstate->InvAvg / pstate->InvAvgCount;
	delete pstate;

	return result;
}
