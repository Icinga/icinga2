// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/invsumaggregator.hpp"

using namespace icinga;

InvSumAggregator::InvSumAggregator(String attr)
	: m_InvSumAttr(std::move(attr))
{ }

InvSumAggregatorState *InvSumAggregator::EnsureState(AggregatorState **state)
{
	if (!*state)
		*state = new InvSumAggregatorState();

	return static_cast<InvSumAggregatorState *>(*state);
}

void InvSumAggregator::Apply(const Table::Ptr& table, const Value& row, AggregatorState **state)
{
	Column column = table->GetColumn(m_InvSumAttr);

	Value value = column.ExtractValue(row);

	InvSumAggregatorState *pstate = EnsureState(state);

	pstate->InvSum += (1.0 / value);
}

double InvSumAggregator::GetResultAndFreeState(AggregatorState *state) const
{
	InvSumAggregatorState *pstate = EnsureState(&state);
	double result = pstate->InvSum;
	delete pstate;

	return result;
}
