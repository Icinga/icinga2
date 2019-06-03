/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef COUNTAGGREGATOR_H
#define COUNTAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct CountAggregatorState final : public AggregatorState
{
	int Count{0};
};

/**
 * @ingroup livestatus
 */
class CountAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(CountAggregator);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	static CountAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* COUNTAGGREGATOR_H */
