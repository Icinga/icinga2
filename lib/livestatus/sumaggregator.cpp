// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
