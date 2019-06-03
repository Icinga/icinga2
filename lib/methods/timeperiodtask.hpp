/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	TimePeriodTask();
};

}

#endif /* TIMEPERIODTASK_H */
