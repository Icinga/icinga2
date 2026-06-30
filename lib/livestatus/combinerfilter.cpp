// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/combinerfilter.hpp"

using namespace icinga;

void CombinerFilter::AddSubFilter(const Filter::Ptr& filter)
{
	m_Filters.push_back(filter);
}
