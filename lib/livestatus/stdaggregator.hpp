// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef STDAGGREGATOR_H
#define STDAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct StdAggregatorState final : public AggregatorState
{
	double StdSum{0};
	double StdQSum{0};
	double StdCount{0};
};

/**
 * @ingroup livestatus
 */
class StdAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(StdAggregator);

	StdAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_StdAttr;

	static StdAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* STDAGGREGATOR_H */
