/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

REGISTER_SCRIPTFUNCTION_NS(Internal, EmptyTimePeriod, &TimePeriodTask::EmptyTimePeriodUpdate);
REGISTER_SCRIPTFUNCTION_NS(Internal, EvenMinutesTimePeriod, &TimePeriodTask::EvenMinutesTimePeriodUpdate);

Array::Ptr TimePeriodTask::EmptyTimePeriodUpdate(const TimePeriod::Ptr&, double, double)
{
	Array::Ptr segments = new Array();
	return segments;
}

Array::Ptr TimePeriodTask::EvenMinutesTimePeriodUpdate(const TimePeriod::Ptr&, double begin, double end)
{
	Array::Ptr segments = new Array();

	for (long t = begin / 60 - 1; t * 60 < end; t++) {
		if ((t % 2) == 0) {
			Dictionary::Ptr segment = new Dictionary();
			segment->Set("begin", t * 60);
			segment->Set("end", (t + 1) * 60);

			segments->Add(segment);
		}
	}

	return segments;
}
