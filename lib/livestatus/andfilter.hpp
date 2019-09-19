/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ANDFILTER_H
#define ANDFILTER_H

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

#endif /* ANDFILTER_H */
