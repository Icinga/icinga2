/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/dependency.hpp"
#include "icinga/service.hpp"
#include "config/configcompilercontext.hpp"
#include "base/logger.hpp"
#include "base/scriptfunction.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Dependency);
REGISTER_SCRIPTFUNCTION(ValidateDependencyFilters, &Dependency::ValidateFilters);

String DependencyNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Dependency::Ptr dependency = dynamic_pointer_cast<Dependency>(context);

	if (!dependency)
		return "";

	String name = dependency->GetChildHostName();

	if (!dependency->GetChildServiceName().IsEmpty())
		name += "!" + dependency->GetChildServiceName();

	name += "!" + shortName;

	return name;
}

void Dependency::OnConfigLoaded(void)
{
	Value defaultFilter;

	if (GetParentServiceName().IsEmpty())
		defaultFilter = StateFilterUp;
	else
		defaultFilter = StateFilterOK | StateFilterWarning;

	SetStateFilter(FilterArrayToInt(GetStates(), defaultFilter));
}

void Dependency::OnStateLoaded(void)
{
	DynamicObject::Start();

	ASSERT(!OwnsLock());

	Host::Ptr childHost = Host::GetByName(GetChildHostName());

	if (childHost) {
		if (GetChildServiceName().IsEmpty()) {
			Log(LogDebug, "Dependency")
			    << "Dependency '" << GetName() << "' child host '" << GetChildHostName() << ".";
			m_Child = childHost;
		} else {
			Log(LogDebug, "Dependency")
			    << "Dependency '" << GetName() << "' child host '" << GetChildHostName() << "' service '" << GetChildServiceName() << "' .";
			m_Child = childHost->GetServiceByShortName(GetChildServiceName());
		}
	}
	
	if (!m_Child)
		Log(LogWarning, "Dependency")
		    << "Dependency '" << GetName() << "' references an invalid child object and will be ignored.";
	else
		m_Child->AddDependency(this);

	Host::Ptr parentHost = Host::GetByName(GetParentHostName());

	if (parentHost) {
		if (GetParentServiceName().IsEmpty()) {
			Log(LogDebug, "Dependency")
			    << "Dependency '" << GetName() << "' parent host '" << GetParentHostName() << ".";
			m_Parent = parentHost;
		} else {
			Log(LogDebug, "Dependency")
			    << "Dependency '" << GetName() << "' parent host '" << GetParentHostName() << "' service '" << GetParentServiceName() << "' .";
			m_Parent = parentHost->GetServiceByShortName(GetParentServiceName());
		}
	}
	
	if (!m_Parent)
		Log(LogWarning, "Dependency")
		    << "Dependency '" << GetName() << "' references an invalid parent object and will always fail.";
	else
		m_Parent->AddReverseDependency(this);
}

void Dependency::Stop(void)
{
	DynamicObject::Stop();

	if (GetChild())
		GetChild()->RemoveDependency(this);

	if (GetParent())
		GetParent()->RemoveReverseDependency(this);
}

bool Dependency::IsAvailable(DependencyType dt) const
{
	Checkable::Ptr parent = GetParent();

	if (!parent)
		return false;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(parent);

	/* ignore if it's the same checkable object */
	if (parent == GetChild()) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: Parent and child " << (service ? "service" : "host") << " are identical.";
		return true;
	}

	/* ignore pending  */
	if (!parent->GetLastCheckResult()) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: " << (service ? "Service" : "Host") << " '" << parent->GetName() << "' hasn't been checked yet.";
		return true;
	}

	/* ignore soft states */
	if (parent->GetStateType() == StateTypeSoft) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: " << (service ? "Service" : "Host") << " '" << parent->GetName() << "' is in a soft state.";
		return true;
	}

	int state;

	if (service)
		state = ServiceStateToFilter(service->GetState());
	else
		state = HostStateToFilter(host->GetState());

	/* check state */
	if (state & GetStateFilter()) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: " << (service ? "Service" : "Host") << " '" << parent->GetName() << "' matches state filter.";
		return true;
	}

	/* ignore if not in time period */
	TimePeriod::Ptr tp = GetPeriod();
	if (tp && !tp->IsInside(Utility::GetTime())) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: Outside time period.";
		return true;
	}

	if (dt == DependencyCheckExecution && !GetDisableChecks()) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: Checks are not disabled.";
		return true;
	} else if (dt == DependencyNotification && !GetDisableNotifications()) {
		Log(LogNotice, "Dependency")
		    << "Dependency '" << GetName() << "' passed: Notifications are not disabled";
		return true;
	}

	Log(LogNotice, "Dependency")
	    << "Dependency '" << GetName() << "' failed. Parent "
	    << (service ? "service" : "host") << " '" << parent->GetName() << "' is "
	    << (service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	return false;
}

Checkable::Ptr Dependency::GetChild(void) const
{
	return m_Child;
}

Checkable::Ptr Dependency::GetParent(void) const
{
	return m_Parent;
}

TimePeriod::Ptr Dependency::GetPeriod(void) const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

void Dependency::ValidateFilters(const String& location, const Dictionary::Ptr& attrs)
{
	int sfilter = FilterArrayToInt(attrs->Get("states"), 0);

	if (attrs->Get("parent_service_name") == Empty && (sfilter & ~(StateFilterUp | StateFilterDown)) != 0) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": State filter is invalid for host dependency.");
	}

	if (attrs->Get("parent_service_name") != Empty && (sfilter & ~(StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0) {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": State filter is invalid for service dependency.");
	}
}
