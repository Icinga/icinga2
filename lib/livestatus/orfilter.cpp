/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/orfilter.hpp"

using namespace icinga;

bool OrFilter::Apply(const Table::Ptr& table, const Value& row)
{
	if (m_Filters.empty())
		return true;

	for (const auto& filter : m_Filters) {
		if (filter->Apply(table, row))
			return true;
	}

	return false;
}
