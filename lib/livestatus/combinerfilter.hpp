// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
