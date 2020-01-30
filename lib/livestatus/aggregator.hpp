/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/i2-livestatus.hpp"
#include "livestatus/table.hpp"
#include "livestatus/filter.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct AggregatorState
{
	virtual ~AggregatorState();
};

/**
 * @ingroup livestatus
 */
class Aggregator : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Aggregator);

	virtual void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) = 0;
	virtual double GetResultAndFreeState(AggregatorState *state) const = 0;
	void SetFilter(const Filter::Ptr& filter);

protected:
	Aggregator() = default;

	Filter::Ptr GetFilter() const;

private:
	Filter::Ptr m_Filter;
};

}
