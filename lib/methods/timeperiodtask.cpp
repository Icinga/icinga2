/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "methods/timeperiodtask.hpp"
#include "base/function.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, EmptyTimePeriod, &TimePeriodTask::EmptyTimePeriodUpdate, "tp:begin:end");
REGISTER_FUNCTION_NONCONST(Internal, EvenMinutesTimePeriod, &TimePeriodTask::EvenMinutesTimePeriodUpdate, "tp:begin:end");

Array::Ptr TimePeriodTask::EmptyTimePeriodUpdate(const TimePeriod::Ptr& tp, double, double)
{
	REQUIRE_NOT_NULL(tp);

	Array::Ptr segments = new Array();
	return segments;
}

Array::Ptr TimePeriodTask::EvenMinutesTimePeriodUpdate(const TimePeriod::Ptr& tp, double begin, double end)
{
	REQUIRE_NOT_NULL(tp);

	ArrayData segments;

	for (long t = begin / 60 - 1; t * 60 < end; t++) {
		if ((t % 2) == 0) {
			segments.push_back(new Dictionary({
				{ "begin", t * 60 },
				{ "end", (t + 1) * 60 }
			}));
		}
	}

	return new Array(std::move(segments));
}
