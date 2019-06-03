/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef COMBINERFILTER_H
#define COMBINERFILTER_H

#include "livestatus/filter.hpp"

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class CombinerFilter : public Filter
{
public:
	DECLARE_PTR_TYPEDEFS(CombinerFilter);

	void AddSubFilter(const Filter::Ptr& filter);

protected:
	std::vector<Filter::Ptr> m_Filters;

	CombinerFilter() = default;
};

}

#endif /* COMBINERFILTER_H */
