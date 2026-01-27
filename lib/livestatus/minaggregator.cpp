// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
