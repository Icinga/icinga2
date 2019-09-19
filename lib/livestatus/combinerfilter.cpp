/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/combinerfilter.hpp"

using namespace icinga;

void CombinerFilter::AddSubFilter(const Filter::Ptr& filter)
{
	m_Filters.push_back(filter);
}
