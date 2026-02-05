// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/andfilter.hpp"

using namespace icinga;

bool AndFilter::Apply(const Table::Ptr& table, const Value& row)
{
	for (const Filter::Ptr& filter : m_Filters) {
		if (!filter->Apply(table, row))
			return false;
	}

	return true;
}
