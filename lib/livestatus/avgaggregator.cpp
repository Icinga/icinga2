// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
