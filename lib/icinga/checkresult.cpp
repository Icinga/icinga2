/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkresult.hpp"
#include "icinga/checkresult-ti.cpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

REGISTER_TYPE(CheckResult);

INITIALIZE_ONCE([]() {
	ScriptGlobal::Set("Icinga.ServiceOK", ServiceOK);
	ScriptGlobal::Set("Icinga.ServiceWarning", ServiceWarning);
	ScriptGlobal::Set("Icinga.ServiceCritical", ServiceCritical);
	ScriptGlobal::Set("Icinga.ServiceUnknown", ServiceUnknown);

	ScriptGlobal::Set("Icinga.HostUp", HostUp);
	ScriptGlobal::Set("Icinga.HostDown", HostDown);
})

double CheckResult::CalculateExecutionTime() const
{
	return GetExecutionEnd() - GetExecutionStart();
}

double CheckResult::CalculateLatency() const
{
	double latency = (GetScheduleEnd() - GetScheduleStart()) - CalculateExecutionTime();

	if (latency < 0)
		latency = 0;

	return latency;
}
