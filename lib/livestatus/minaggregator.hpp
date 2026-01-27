// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MINAGGREGATOR_H
#define MINAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"
#include <cfloat>

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct MinAggregatorState final : public AggregatorState
{
	double Min{DBL_MAX};
};

/**
 * @ingroup livestatus
 */
class MinAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(MinAggregator);

	MinAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_MinAttr;

	static MinAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* MINAGGREGATOR_H */
