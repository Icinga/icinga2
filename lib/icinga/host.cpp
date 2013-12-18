/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "icinga/host.h"
#include "icinga/service.h"
#include "icinga/hostgroup.h"
#include "icinga/icingaapplication.h"
#include "icinga/pluginutility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/scriptfunction.h"
#include "base/debug.h"
#include "base/serializer.h"
#include "config/configitembuilder.h"
#include "config/configcompilercontext.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Host);

void Host::Start(void)
{
	DynamicObject::Start();

	ASSERT(!OwnsLock());

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->AddMember(GetSelf());
		}
	}
}

void Host::OnConfigLoaded(void)
{
	UpdateSlaveServices();
}

void Host::Stop(void)
{
	DynamicObject::Stop();

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			if (hg)
				hg->RemoveMember(GetSelf());
		}
	}

	// TODO: unregister slave services/notifications?
}

bool Host::IsReachable(void) const
{
	ASSERT(!OwnsLock());

	std::set<Service::Ptr> parentServices = GetParentServices();

	BOOST_FOREACH(const Service::Ptr& service, parentServices) {
		ObjectLock olock(service);

		/* ignore pending services */
		if (!service->GetLastCheckResult())
			continue;

		/* ignore soft states */
		if (service->GetStateType() == StateTypeSoft)
			continue;

		/* ignore services states OK and Warning */
		if (service->GetState() == StateOK ||
		    service->GetState() == StateWarning)
			continue;

		return false;
	}

	std::set<Host::Ptr> parentHosts = GetParentHosts();

	BOOST_FOREACH(const Host::Ptr& host, parentHosts) {
		Service::Ptr hc = host->GetCheckService();

		/* ignore hosts that don't have a check */
		if (!hc)
			continue;

		ObjectLock olock(hc);

		/* ignore soft states */
		if (hc->GetStateType() == StateTypeSoft)
			continue;

		/* ignore hosts that are up */
		if (hc->GetState() == StateOK)
			continue;

		return false;
	}

	return true;
}

void Host::UpdateSlaveServices(void)
{
	ASSERT(!OwnsLock());

	Dictionary::Ptr service_descriptions = GetServiceDescriptions();

	if (!service_descriptions ||service_descriptions->GetLength() == 0)
		return;

	ConfigItem::Ptr item = ConfigItem::GetObject("Host", GetName());

	ObjectLock olock(service_descriptions);
	BOOST_FOREACH(const Dictionary::Pair& kv, service_descriptions) {
		std::ostringstream namebuf;
		namebuf << GetName() << "!" << kv.first;
		String name = namebuf.str();

		std::vector<String> path;
		path.push_back("services");
		path.push_back(kv.first);

		ExpressionList::Ptr exprl;

		{
			ObjectLock ilock(item);

			exprl = item->GetLinkedExpressionList();
		}

		DebugInfo di;
		exprl->FindDebugInfoPath(path, di);

		if (di.Path.IsEmpty())
			di = item->GetDebugInfo();

		ConfigItemBuilder::Ptr builder = make_shared<ConfigItemBuilder>(di);
		builder->SetType("Service");
		builder->SetName(name);
		builder->AddExpression("host", OperatorSet, GetName());
		builder->AddExpression("display_name", OperatorSet, kv.first);
		builder->AddExpression("short_name", OperatorSet, kv.first);

		if (!kv.second.IsObjectType<Dictionary>())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Service description must be either a string or a dictionary."));

		Dictionary::Ptr service = kv.second;

		Array::Ptr templates = service->Get("templates");

		if (templates) {
			ObjectLock olock(templates);

			BOOST_FOREACH(const Value& tmpl, templates) {
				builder->AddParent(tmpl);
			}
		}

		/* Clone attributes from the service expression list. */
		ExpressionList::Ptr svc_exprl = make_shared<ExpressionList>();
		exprl->ExtractPath(path, svc_exprl);

		builder->AddExpressionList(svc_exprl);

		ConfigItem::Ptr serviceItem = builder->Compile();
		serviceItem->Register();
		DynamicObject::Ptr dobj = serviceItem->Commit();
		dobj->OnConfigLoaded();
	}
}

std::set<Service::Ptr> Host::GetServices(void) const
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	std::set<Service::Ptr> services;
	typedef std::pair<String, Service::Ptr> ServicePair;
	BOOST_FOREACH(const ServicePair& kv, m_Services) {
		services.insert(kv.second);
	}

	return services;
}

void Host::AddService(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	m_Services[service->GetShortName()] = service;
}

void Host::RemoveService(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_ServicesMutex);

	m_Services.erase(service->GetShortName());
}

int Host::GetTotalServices(void) const
{
	return GetServices().size();
}

Service::Ptr Host::GetServiceByShortName(const Value& name) const
{
	if (name.IsScalar()) {
		{
			boost::mutex::scoped_lock lock(m_ServicesMutex);

			std::map<String, Service::Ptr>::const_iterator it = m_Services.find(name);

			if (it != m_Services.end())
				return it->second;
		}

		return Service::Ptr();
	} else if (name.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = name;
		String short_name;

		return Service::GetByNamePair(dict->Get("host"), dict->Get("service"));
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Host/Service name pair is invalid: " + JsonSerialize(name)));
	}
}

std::set<Host::Ptr> Host::GetParentHosts(void) const
{
	std::set<Host::Ptr> parents;

	Array::Ptr dependencies = GetHostDependencies();

	if (dependencies) {
		ObjectLock olock(dependencies);

		BOOST_FOREACH(const Value& value, dependencies) {
			if (value == GetName())
				continue;

			Host::Ptr host = GetByName(value);

			parents.insert(host);
		}
	}

	return parents;
}

std::set<Host::Ptr> Host::GetChildHosts(void) const
{
	std::set<Host::Ptr> children;

        BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
		Array::Ptr dependencies = host->GetHostDependencies();

		if (dependencies) {
			ObjectLock olock(dependencies);

			BOOST_FOREACH(const Value& value, dependencies) {
				if (value == GetName())
					children.insert(host);
			}
		}
	}

	return children;
}

Service::Ptr Host::GetCheckService(void) const
{
	String host_check = GetCheck();

	if (host_check.IsEmpty())
		return Service::Ptr();

	return GetServiceByShortName(host_check);
}

std::set<Service::Ptr> Host::GetParentServices(void) const
{
	std::set<Service::Ptr> parents;

	Array::Ptr dependencies = GetServiceDependencies();

	if (dependencies) {
		ObjectLock olock(dependencies);

		BOOST_FOREACH(const Value& value, dependencies) {
			parents.insert(GetServiceByShortName(value));
		}
	}

	return parents;
}

HostState Host::CalculateState(ServiceState state, bool reachable)
{
	if (!reachable)
		return HostUnreachable;

	switch (state) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

HostState Host::GetState(void) const
{
	ASSERT(!OwnsLock());

	if (!IsReachable())
		return HostUnreachable;

	Service::Ptr hc = GetCheckService();

	if (!hc)
		return HostUp;

	switch (hc->GetState()) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}

}

HostState Host::GetLastState(void) const
{
	ASSERT(!OwnsLock());

	if (!IsReachable())
		return HostUnreachable;

	Service::Ptr hc = GetCheckService();

	if (!hc)
		return HostUp;

	switch (hc->GetLastState()) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

HostState Host::GetLastHardState(void) const
{
	ASSERT(!OwnsLock());

	if (!IsReachable())
		return HostUnreachable;

	Service::Ptr hc = GetCheckService();

	if (!hc)
		return HostUp;

	switch (hc->GetLastHardState()) {
		case StateOK:
		case StateWarning:
			return HostUp;
		default:
			return HostDown;
	}
}

double Host::GetLastStateUp(void) const
{
	ASSERT(!OwnsLock());

	Service::Ptr hc = GetCheckService();

	if (!hc)
		return 0;

	if (hc->GetLastStateOK() > hc->GetLastStateWarning())
		return hc->GetLastStateOK();
	else
		return hc->GetLastStateWarning();
}

double Host::GetLastStateDown(void) const
{
	ASSERT(!OwnsLock());

	Service::Ptr hc = GetCheckService();

	if (!hc)
		return 0;

	return hc->GetLastStateCritical();
}

double Host::GetLastStateUnreachable(void) const
{
	ASSERT(!OwnsLock());

	Service::Ptr hc = GetCheckService();

	if (!hc)
		return 0;

	return hc->GetLastStateUnreachable();
}

double Host::GetLastStateChange(void) const
{
	Service::Ptr hc = GetCheckService();

	if (!hc)
		return Application::GetStartTime();

	return hc->GetLastStateChange();
}


double Host::GetLastHardStateChange(void) const
{
	Service::Ptr hc = GetCheckService();

	if (!hc)
		return Application::GetStartTime();

	return hc->GetLastHardStateChange();
}

StateType Host::GetLastStateType(void) const
{
	Service::Ptr hc = GetCheckService();

	if (!hc)
		return StateTypeHard;

	return hc->GetLastStateType();
}

StateType Host::GetStateType(void) const
{
	Service::Ptr hc = GetCheckService();

	if (!hc)
		return StateTypeHard;

	return hc->GetStateType();
}

HostState Host::StateFromString(const String& state)
{
	if (state == "UP")
		return HostUp;
	else if (state == "DOWN")
		return HostDown;
	else if (state == "UNREACHABLE")
		return HostUnreachable;
	else
		return HostUnreachable;
}

String Host::StateToString(HostState state)
{
	switch (state) {
		case HostUp:
			return "UP";
		case HostDown:
			return "DOWN";
		case HostUnreachable:
			return "UNREACHABLE";
		default:
			return "INVALID";
	}
}

StateType Host::StateTypeFromString(const String& type)
{
	if (type == "SOFT")
		return StateTypeSoft;
	else
		return StateTypeHard;
}

String Host::StateTypeToString(StateType type)
{
	if (type == StateTypeSoft)
		return "SOFT";
	else
		return "HARD";
}

bool Host::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	if (macro == "HOSTNAME") {
		*result = GetName();
		return true;
	}
	else if (macro == "HOSTDISPLAYNAME" || macro == "HOSTALIAS") {
		*result = GetDisplayName();
		return true;
	}

	Service::Ptr hc = GetCheckService();
	CheckResult::Ptr hccr;

	if (hc) {
		ServiceState state = hc->GetState();
		bool reachable = IsReachable();

		if (macro == "HOSTSTATE") {
			HostState hstate = CalculateState(state, reachable);

			switch (hstate) {
				case HostUnreachable:
					*result = "UNREACHABLE";
					break;
				case HostUp:
					*result = "UP";
					break;
				case HostDown:
					*result = "DOWN";
					break;
				default:
					ASSERT(0);
			}

			return true;
		} else if (macro == "HOSTSTATEID") {
			*result = Convert::ToString(state);
			return true;
		} else if (macro == "HOSTSTATETYPE") {
			*result = Service::StateTypeToString(hc->GetStateType());
			return true;
		} else if (macro == "HOSTATTEMPT") {
			*result = Convert::ToString(hc->GetCheckAttempt());
			return true;
		} else if (macro == "MAXHOSTATTEMPT") {
			*result = Convert::ToString(hc->GetMaxCheckAttempts());
			return true;
		} else if (macro == "LASTHOSTSTATE") {
			*result = StateToString(GetLastState());
			return true;
		} else if (macro == "LASTHOSTSTATEID") {
			*result = Convert::ToString(GetLastState());
			return true;
		} else if (macro == "LASTHOSTSTATETYPE") {
			*result = Service::StateTypeToString(GetLastStateType());
			return true;
		} else if (macro == "LASTHOSTSTATECHANGE") {
			*result = Convert::ToString((long)hc->GetLastStateChange());
			return true;
		} else if (macro == "HOSTDURATIONSEC") {
			*result = Convert::ToString((long)(Utility::GetTime() - hc->GetLastStateChange()));
			return true;
		}

		hccr = hc->GetLastCheckResult();
	}

	if (hccr) {
		if (macro == "HOSTLATENCY") {
			*result = Convert::ToString(Service::CalculateLatency(hccr));
			return true;
		} else if (macro == "HOSTEXECUTIONTIME") {
			*result = Convert::ToString(Service::CalculateExecutionTime(hccr));
			return true;
		} else if (macro == "HOSTOUTPUT") {
			*result = hccr->GetOutput();
			return true;
		} else if (macro == "HOSTPERFDATA") {
			*result = PluginUtility::FormatPerfdata(hccr->GetPerformanceData());
			return true;
		} else if (macro == "LASTHOSTCHECK") {
			*result = Convert::ToString((long)hccr->GetScheduleStart());
			return true;
		}
	}

	if (macro.SubStr(0, 5) == "_HOST") {
		Dictionary::Ptr custom = GetCustom();
		*result = custom ? custom->Get(macro.SubStr(5)) : "";
		return true;
	}

	Dictionary::Ptr macros = GetMacros();

	String name = macro;

	if (name == "HOSTADDRESS")
		name = "address";
	else if (macro == "HOSTADDRESS6")
		name = "address6";

	if (macros && macros->Contains(name)) {
		*result = macros->Get(name);
		return true;
	}

	if (macro == "HOSTADDRESS" || macro == "HOSTADDRESS6") {
		*result = GetName();
		return true;
	}

	return false;
}
