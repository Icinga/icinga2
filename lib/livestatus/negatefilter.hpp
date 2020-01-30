/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/filter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class NegateFilter final : public Filter
{
public:
	DECLARE_PTR_TYPEDEFS(NegateFilter);

	NegateFilter(Filter::Ptr inner);

	bool Apply(const Table::Ptr& table, const Value& row) override;

private:
	Filter::Ptr m_Inner;
};

}
