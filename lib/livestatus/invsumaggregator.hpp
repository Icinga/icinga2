/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef INVSUMAGGREGATOR_H
#define INVSUMAGGREGATOR_H

#include "livestatus/aggregator.hpp"
#include "livestatus/table.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct InvSumAggregatorState final : public AggregatorState
{
	double InvSum{0};
};

/**
 * @ingroup livestatus
 */
class InvSumAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(InvSumAggregator);

	InvSumAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_InvSumAttr;

	static InvSumAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* INVSUMAGGREGATOR_H */
