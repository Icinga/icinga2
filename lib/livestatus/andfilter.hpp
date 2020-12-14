/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/combinerfilter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class AndFilter final : public CombinerFilter
{
public:
	DECLARE_PTR_TYPEDEFS(AndFilter);

	bool Apply(const Table::Ptr& table, const Value& row) override;
};

}
