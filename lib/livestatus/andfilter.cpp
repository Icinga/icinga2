/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/andfilter.hpp"

using namespace icinga;

bool AndFilter::Apply(const Table::Ptr& table, const Value& row)
{
	for (const auto& filter : m_Filters) {
		if (!filter->Apply(table, row))
			return false;
	}

	return true;
}
