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
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"
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

void Dependency::OnAllConfigLoaded(void)
{
	DynamicObject::OnAllConfigLoaded();

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
		BOOST_THROW_EXCEPTION(ScriptError("Dependency '" << GetName() << "' references a child host/service which doesn't exist.", GetDebugInfo()));

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
		BOOST_THROW_EXCEPTION(ScriptError("Dependency '" << GetName() << "' references a parent host/service which doesn't exist.", GetDebugInfo()));

	m_Parent->AddReverseDependency(this);
}

void Dependency::Stop(void)
{
	DynamicObject::Stop();

	GetChild()->RemoveDependency(this);
	GetParent()->RemoveReverseDependency(this);
}

bool Dependency::IsAvailable(DependencyType dt) const
{
	Checkable::Ptr parent = GetParent();

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

void Dependency::ValidateFilters(const String& location, const Dependency::Ptr& object)
{
	int sfilter = FilterArrayToInt(object->GetStates(), 0);

	if (object->GetParentServiceName().IsEmpty() && (sfilter & ~(StateFilterUp | StateFilterDown)) != 0) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": State filter is invalid for host dependency.", object->GetDebugInfo()));
	}

	if (!object->GetParentServiceName().IsEmpty() && (sfilter & ~(StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": State filter is invalid for service dependency.", object->GetDebugInfo()));
	}
}
