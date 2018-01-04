/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef AVGAGGREGATOR_H
#define AVGAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct AvgAggregatorState final : public AggregatorState
{
	AvgAggregatorState()
		: Avg(0), AvgCount(0)
	{ }

	double Avg;
	double AvgCount;
};

/**
 * @ingroup livestatus
 */
class AvgAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(AvgAggregator);

	AvgAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_AvgAttr;

	static AvgAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* AVGAGGREGATOR_H */
