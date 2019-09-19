/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ORFILTER_H
#define ORFILTER_H

#include "livestatus/combinerfilter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class OrFilter final : public CombinerFilter
{
public:
	DECLARE_PTR_TYPEDEFS(OrFilter);

	bool Apply(const Table::Ptr& table, const Value& row) override;
};

}

#endif /* ORFILTER_H */
