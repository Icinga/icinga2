/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/timeperiodtask.hpp"
#include "base/function.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, EmptyTimePeriod, &TimePeriodTask::EmptyTimePeriodUpdate, "tp:begin:end");
REGISTER_FUNCTION_NONCONST(Internal, EvenMinutesTimePeriod, &TimePeriodTask::EvenMinutesTimePeriodUpdate, "tp:begin:end");

Array::Ptr TimePeriodTask::EmptyTimePeriodUpdate(const TimePeriod::Ptr& tp, double, double)
{
	REQUIRE_NOT_NULL(tp);

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = "";
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = 0;

		Checkable::ExecuteCommandProcessFinishedHandler("", pr);
	}

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

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = "";
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = 0;

		Checkable::ExecuteCommandProcessFinishedHandler("", pr);
	}

	return new Array(std::move(segments));
}
