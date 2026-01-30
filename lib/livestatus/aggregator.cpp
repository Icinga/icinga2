// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "livestatus/aggregator.hpp"

using namespace icinga;

void Aggregator::SetFilter(const Filter::Ptr& filter)
{
	m_Filter = filter;
}

Filter::Ptr Aggregator::GetFilter() const
{
	return m_Filter;
}

AggregatorState::~AggregatorState()
{ }
