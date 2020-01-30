/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct AvgAggregatorState final : public AggregatorState
{
	double Avg{0};
	double AvgCount{0};
};

/**
 * @ingroup livestatus
 */
class AvgAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(AvgAggregator);

	AvgAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_AvgAttr;

	static AvgAggregatorState *EnsureState(AggregatorState **state);
};

}
