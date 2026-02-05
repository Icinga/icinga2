// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/negatefilter.hpp"

using namespace icinga;

NegateFilter::NegateFilter(Filter::Ptr inner)
	: m_Inner(std::move(inner))
{ }

bool NegateFilter::Apply(const Table::Ptr& table, const Value& row)
{
	return !m_Inner->Apply(table, row);
}
