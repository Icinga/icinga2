// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
