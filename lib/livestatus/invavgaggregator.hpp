// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INVAVGAGGREGATOR_H
#define INVAVGAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct InvAvgAggregatorState final : public AggregatorState
{
	double InvAvg{0};
	double InvAvgCount{0};
};

/**
 * @ingroup livestatus
 */
class InvAvgAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(InvAvgAggregator);

	InvAvgAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_InvAvgAttr;

	static InvAvgAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* INVAVGAGGREGATOR_H */
