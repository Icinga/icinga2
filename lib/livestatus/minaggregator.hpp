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

#ifndef MINAGGREGATOR_H
#define MINAGGREGATOR_H

#include "livestatus/table.hpp"
#include "livestatus/aggregator.hpp"
#include <cfloat>

namespace icinga
{

/**
 * @ingroup livestatus
 */
struct MinAggregatorState final : public AggregatorState
{
	MinAggregatorState()
		: Min(DBL_MAX)
	{ }

	double Min;
};

/**
 * @ingroup livestatus
 */
class MinAggregator final : public Aggregator
{
public:
	DECLARE_PTR_TYPEDEFS(MinAggregator);

	MinAggregator(const String& attr);

	void Apply(const Table::Ptr& table, const Value& row, AggregatorState **state) override;
	double GetResultAndFreeState(AggregatorState *state) const override;

private:
	String m_MinAttr;

	static MinAggregatorState *EnsureState(AggregatorState **state);
};

}

#endif /* MINAGGREGATOR_H */
