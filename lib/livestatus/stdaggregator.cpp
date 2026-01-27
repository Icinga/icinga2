// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
