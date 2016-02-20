/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef TIMEPERIODTASK_H
#define TIMEPERIODTASK_H

#include "icinga/timeperiod.hpp"

namespace icinga
{

/**
* Test timeperiod functions.
*
* @ingroup methods
*/
class TimePeriodTask
{
public:
	static Array::Ptr EmptyTimePeriodUpdate(const TimePeriod::Ptr& tp, double begin, double end);
	static Array::Ptr EvenMinutesTimePeriodUpdate(const TimePeriod::Ptr& tp, double begin, double end);

private:
	TimePeriodTask(void);
};

}

#endif /* TIMEPERIODTASK_H */
