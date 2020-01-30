/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
