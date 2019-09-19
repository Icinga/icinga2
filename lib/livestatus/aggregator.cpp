/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/aggregator.hpp"

using namespace icinga;

void Aggregator::SetFilter(const Filter::Ptr& filter)
{
	m_Filter = filter;
}

Filter::Ptr Aggregator::GetFilter() const
{
	return m_Filter;
}

AggregatorState::~AggregatorState()
{ }
