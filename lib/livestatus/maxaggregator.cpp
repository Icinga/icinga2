// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
