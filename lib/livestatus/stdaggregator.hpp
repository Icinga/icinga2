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

#ifndef STDAGGREGATOR_H
#define STDAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct StdAggregatorState final : public AggregatorState
{
	StdAggregatorState()
		: StdSum(0), StdQSum(0), StdCount(0)
	{ }

	double StdSum;
	double StdQSum;
	double StdCount;
};

/**
 * @ingroup livestatus
 */
class StdAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(StdAggregator);

	StdAggregator(String attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_StdAttr;

	static StdAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* STDAGGREGATOR_H */
