/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct SumAggregatorState final : public AggregatorState
{
	double Sum{0};
};

/**
 * @ingroup livestatus
 */
class SumAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(SumAggregator);

	SumAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_SumAttr;

	static SumAggregatorState *EnsureState(AggregatorState **state);
};

}
