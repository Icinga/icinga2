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

#include "livestatus/minaggregator.hpp"

using namespace icinga;

MinAggregator::MinAggregator(String attr)
	: m_MinAttr(std::move(attr))
{ }

MinAggregatorState *MinAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new MinAggregatorState();

	return static_cast<MinAggregatorState *>(*state);
}

void MinAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_MinAttr);

	Value value = column.ExtractValue(row);

	MinAggregatorState *pstate = EnsureState(state);

	if (value < pstate->Min)
		pstate->Min = value;
}

double MinAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	MinAggregatorState *pstate = EnsureState(&state);

	double result;

	if (pstate->Min == DBL_MAX)
		result = 0;
	else
		result = pstate->Min;

	delete pstate;

	return result;
}
