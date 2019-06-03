/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkresult.hpp"
#include "icinga/checkresult-ti.cpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

REGISTER_TYPE(CheckResult);

INITIALIZE_ONCE([]() {
	ScriptGlobal::Set("Icinga.ServiceOK", ServiceOK, true);
	ScriptGlobal::Set("Icinga.ServiceWarning", ServiceWarning, true);
	ScriptGlobal::Set("Icinga.ServiceCritical", ServiceCritical, true);
	ScriptGlobal::Set("Icinga.ServiceUnknown", ServiceUnknown, true);

	ScriptGlobal::Set("Icinga.HostUp", HostUp, true);
	ScriptGlobal::Set("Icinga.HostDown", HostDown, true);
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
