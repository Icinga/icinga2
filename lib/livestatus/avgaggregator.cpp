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

#include "livestatus/avgaggregator.hpp"

using namespace icinga;

AvgAggregator::AvgAggregator(String attr)
	: m_AvgAttr(std::move(attr))
{ }

AvgAggregatorState *AvgAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new AvgAggregatorState();

	return static_cast<AvgAggregatorState *>(*state);
}

void AvgAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_AvgAttr);

	Value value = column.ExtractValue(row);

	AvgAggregatorState *pstate = EnsureState(state);

	pstate->Avg += value;
	pstate->AvgCount++;
}

double AvgAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	AvgAggregatorState *pstate = EnsureState(&state);
	double result = pstate->Avg / pstate->AvgCount;
	delete pstate;

	return result;
}
