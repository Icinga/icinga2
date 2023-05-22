/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
