/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-present Icinga Development Team (http://www.icinga.org) *
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

#include "icinga/dependency.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Dependency);

void Dependency::OnStateLoaded(void)
{
	DynamicObject::Start();

	ASSERT(!OwnsLock());

	if (!GetChildService())
		Log(LogWarning, "icinga", "Dependency '" + GetName() + "' references an invalid child service and will be ignored.");
	else
		GetChildService()->AddDependency(GetSelf());

	if (!GetParentService())
		Log(LogWarning, "icinga", "Dependency '" + GetName() + "' references an invalid parent service and will always fail.");
	else
		GetParentService()->AddReverseDependency(GetSelf());
}

void Dependency::Stop(void)
{
	DynamicObject::Stop();

	if (GetChildService())
		GetChildService()->RemoveDependency(GetSelf());

	if (GetParentService())
		GetParentService()->RemoveReverseDependency(GetSelf());
}

bool Dependency::IsAvailable(DependencyType dt) const
{
	Service::Ptr service = GetParentService();

	if (!service)
		return false;

	/* ignore if it's the same service */
	if (service == GetChildService()) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Parent and child service are identical.");
		return true;
	}

	/* ignore pending services */
	if (!service->GetLastCheckResult()) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Service hasn't been checked yet.");
		return true;
	}

	/* ignore soft states */
	if (service->GetStateType() == StateTypeSoft) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Service is in a soft state.");
		return true;
	}

	/* check state */
	if ((1 << static_cast<int>(service->GetState())) & GetStateFilter()) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Service matches state filter.");
		return true;
	}

	/* ignore if not in time period */
	TimePeriod::Ptr tp = GetPeriod();
	if (tp && !tp->IsInside(Utility::GetTime())) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Outside time period.");
		return true;
	}

	if (dt == DependencyCheckExecution && !GetDisableChecks()) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Checks are not disabled.");
		return true;
	} else if (dt == DependencyNotification && !GetDisableNotifications()) {
		Log(LogDebug, "icinga", "Dependency '" + GetName() + "' passed: Notifications are not disabled");
		return true;
	}

	Log(LogDebug, "icinga", "Dependency '" + GetName() + "' failed.");
	return false;
}

Service::Ptr Dependency::GetChildService(void) const
{
	Host::Ptr host = Host::GetByName(GetChildHostRaw());

	if (!host)
		return Service::Ptr();

	if (GetChildServiceRaw().IsEmpty())
		return host->GetCheckService();

	return host->GetServiceByShortName(GetChildServiceRaw());
}

Service::Ptr Dependency::GetParentService(void) const
{
	Host::Ptr host = Host::GetByName(GetParentHostRaw());

	if (!host)
		return Service::Ptr();

	if (GetParentServiceRaw().IsEmpty())
		return host->GetCheckService();

	return host->GetServiceByShortName(GetParentServiceRaw());
}

TimePeriod::Ptr Dependency::GetPeriod(void) const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}
