// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MAXAGGREGATOR_H
#define MAXAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct MaxAggregatorState final : public AggregatorState
{
	double Max{0};
};

/**
 * @ingroup livestatus
 */
class MaxAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(MaxAggregator);

	MaxAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_MaxAttr;

	static MaxAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* MAXAGGREGATOR_H */
