// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/countaggregator.hpp"

using namespace icinga;

CountAggregatorState *CountAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new CountAggregatorState();

	return static_cast<CountAggregatorState *>(*state);
}

void CountAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	CountAggregatorState *pstate = EnsureState(state);

	if (GetFilter()->Apply(table, row))
		pstate->Count++;
}

double CountAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	CountAggregatorState *pstate = EnsureState(&state);
	double result = pstate->Count;
	delete pstate;

	return result;
}
