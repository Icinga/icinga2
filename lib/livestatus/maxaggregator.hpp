/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef MAXAGGREGATOR_H
#define MAXAGGREGATOR_H

#include "livestatus/aggregator.hpp"
#include "livestatus/table.hpp"

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
