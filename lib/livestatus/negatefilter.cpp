/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/negatefilter.hpp"

using namespace icinga;

NegateFilter::NegateFilter(Filter::Ptr inner)
	: m_Inner(std::move(inner))
{ }

bool NegateFilter::Apply(const Table::Ptr& table, const Value& row)
{
	return !m_Inner->Apply(table, row);
}
