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

#include "livestatus/stdaggregator.hpp"
#include <math.h>

using namespace icinga;

StdAggregator::StdAggregator(String attr)
	: m_StdAttr(std::move(attr))
{ }

StdAggregatorState *StdAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new StdAggregatorState();

	return static_cast<StdAggregatorState *>(*state);
}

void StdAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_StdAttr);

	Value value = column.ExtractValue(row);

	StdAggregatorState *pstate = EnsureState(state);

	pstate->StdSum += value;
	pstate->StdQSum += pow(value, 2);
	pstate->StdCount++;
}

double StdAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	StdAggregatorState *pstate = EnsureState(&state);
	double result = sqrt((pstate->StdQSum - (1 / pstate->StdCount) * pow(pstate->StdSum, 2)) / (pstate->StdCount - 1));
	delete pstate;

	return result;
}
