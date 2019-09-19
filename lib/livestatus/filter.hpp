/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef FILTER_H
#define FILTER_H

#include "livestatus/i2-livestatus.hpp"
#include "livestatus/table.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
class Filter : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Filter);

	virtual bool Apply(const Table::Ptr& table, const Value& row) = 0;

protected:
	Filter() = default;
};

}

#endif /* FILTER_H */
