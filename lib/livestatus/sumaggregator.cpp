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

#include "livestatus/sumaggregator.hpp"

using namespace icinga;

SumAggregator::SumAggregator(String attr)
	: m_SumAttr(std::move(attr))
{ }

SumAggregatorState *SumAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new SumAggregatorState();

	return static_cast<SumAggregatorState *>(*state);
}

void SumAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_SumAttr);

	Value value = column.ExtractValue(row);

	SumAggregatorState *pstate = EnsureState(state);

	pstate->Sum += value;
}

double SumAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	SumAggregatorState *pstate = EnsureState(&state);
	double result = pstate->Sum;
	delete pstate;

	return result;
}
