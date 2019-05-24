/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/sleepchecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/pluginutility.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, SleepCheck, &SleepCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void SleepCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
        const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
    REQUIRE_NOT_NULL(checkable);
    REQUIRE_NOT_NULL(cr);

    CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

    Host::Ptr host;
    Service::Ptr service;
    tie(host, service) = GetHostService(checkable);

    MacroProcessor::ResolverList resolvers;
    if (service)
        resolvers.emplace_back("service", service);
    resolvers.emplace_back("host", host);
    resolvers.emplace_back("command", commandObj);
    resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

    double sleepTime = MacroProcessor::ResolveMacros("$sleep_time$", resolvers, checkable->GetLastCheckResult(),
            nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

    if (resolvedMacros && !useResolvedMacros)
        return;

    Utility::Sleep(sleepTime);

    String output = "Slept for " + Convert::ToString(sleepTime) + " seconds.";

    double now = Utility::GetTime();

    cr->SetOutput(output);
    cr->SetExecutionStart(now);
    cr->SetExecutionEnd(now);

    checkable->ProcessCheckResult(cr);
}