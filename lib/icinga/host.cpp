/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/convert.h"
#include "base/scriptfunction.h"
#include "base/utility.h"
#include "config/configitembuilder.h"
#include "config/configcompilercontext.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static boost::mutex l_ServiceMutex;
static std::map<String, std::map<String, Service::WeakPtr> > l_ServicesCache;
static bool l_ServicesCacheNeedsUpdate = false;
static Timer::Ptr l_ServicesCacheTimer;

REGISTER_SCRIPTFUNCTION(ValidateServiceDictionary, &Host::ValidateServiceDictionary);

REGISTER_TYPE(Host);

Host::Host(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("display_name", Attribute_Config, &m_DisplayName);
	RegisterAttribute("hostgroups", Attribute_Config, &m_HostGroups);
	RegisterAttribute("macros", Attribute_Config, &m_Macros);
	RegisterAttribute("hostdependencies", Attribute_Config, &m_HostDependencies);
	RegisterAttribute("servicedependencies", Attribute_Config, &m_ServiceDependencies);
	RegisterAttribute("hostcheck", Attribute_Config, &m_HostCheck);

}

Host::~Host(void)
{
	HostGroup::InvalidateMembersCache();

	if (m_SlaveServices) {
		ConfigItem::Ptr service;
		BOOST_FOREACH(boost::tie(boost::tuples::ignore, service), m_SlaveServices) {
			service->Unregister();
		}
	}
}

void Host::OnRegistrationCompleted(void)
{
	ASSERT(!OwnsLock());

	DynamicObject::OnRegistrationCompleted();

	Host::InvalidateServicesCache();
	UpdateSlaveServices();
}

String Host::GetDisplayName(void) const
{
	if (!m_DisplayName.IsEmpty())
		return m_DisplayName;
	else
		return GetName();
}

Host::Ptr Host::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Host", name);

	return dynamic_pointer_cast<Host>(configObject);
}

Array::Ptr Host::GetGroups(void) const
{
	return m_HostGroups;
}

Dictionary::Ptr Host::GetMacros(void) const
{
	return m_Macros;
}

Array::Ptr Host::GetHostDependencies(void) const
{
	return m_HostDependencies;;
}

Array::Ptr Host::GetServiceDependencies(void) const
{
	return m_ServiceDependencies;
}

String Host::GetHostCheck(void) const
{
	return m_HostCheck;
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
		Service::Ptr hc = host->GetHostCheckService();

		/* ignore hosts that don't have a hostcheck */
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

	ConfigItem::Ptr item = ConfigItem::GetObject("Host", GetName());

	/* Don't create slave services unless we own this object */
	if (!item)
		return;

	Dictionary::Ptr oldServices = m_SlaveServices;
	Dictionary::Ptr serviceDescs = Get("services");

	Dictionary::Ptr newServices = boost::make_shared<Dictionary>();

	if (serviceDescs) {
		ObjectLock olock(serviceDescs);
		String svcname;
		Value svcdesc;
		BOOST_FOREACH(boost::tie(svcname, svcdesc), serviceDescs) {
			if (svcdesc.IsScalar())
				svcname = svcdesc;

			std::ostringstream namebuf;
			namebuf << GetName() << ":" << svcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("Service");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, GetName());
			builder->AddExpression("display_name", OperatorSet, svcname);
			builder->AddExpression("short_name", OperatorSet, svcname);

			if (!svcdesc.IsObjectType<Dictionary>())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Service description must be either a string or a dictionary."));

			Dictionary::Ptr service = svcdesc;

			Array::Ptr templates = service->Get("templates");

			if (templates) {
				ObjectLock olock(templates);

				BOOST_FOREACH(const Value& tmpl, templates) {
					builder->AddParent(tmpl);
				}
			}

			/* Clone attributes from the host object. */
			std::set<String, string_iless> keys;
			keys.insert("check_interval");
			keys.insert("retry_interval");
			keys.insert("servicegroups");
			keys.insert("checkers");
			keys.insert("notification_interval");
			keys.insert("notification_type_filter");
			keys.insert("notification_state_filter");
			keys.insert("check_period");
			keys.insert("servicedependencies");
			keys.insert("hostdependencies");
			keys.insert("export_macros");

			ExpressionList::Ptr host_exprl = boost::make_shared<ExpressionList>();
			item->GetLinkedExpressionList()->ExtractFiltered(keys, host_exprl);
			builder->AddExpressionList(host_exprl);

			/* Clone attributes from the service expression list. */
			std::vector<String> path;
			path.push_back("services");
			path.push_back(svcname);

			ExpressionList::Ptr svc_exprl = boost::make_shared<ExpressionList>();
			item->GetLinkedExpressionList()->ExtractPath(path, svc_exprl);
			builder->AddExpressionList(svc_exprl);

			ConfigItem::Ptr serviceItem = builder->Compile();
			DynamicObject::Ptr dobj = serviceItem->Commit();

			newServices->Set(name, serviceItem);
		}
	}

	if (oldServices) {
		ObjectLock olock(oldServices);

		ConfigItem::Ptr service;
		BOOST_FOREACH(boost::tie(boost::tuples::ignore, service), oldServices) {
			if (!service)
				continue;

			if (!newServices->Contains(service->GetName()))
				service->Unregister();
		}
	}

	newServices->Seal();

	Set("slave_services", newServices);
}

void Host::OnAttributeChanged(const String& name)
{
	ASSERT(!OwnsLock());

	if (name == "hostgroups")
		HostGroup::InvalidateMembersCache();
	else if (name == "services") {
		UpdateSlaveServices();
	} else if (name == "notifications") {
		BOOST_FOREACH(const Service::Ptr& service, GetServices()) {
			service->UpdateSlaveNotifications();
		}
	}
}

std::set<Service::Ptr> Host::GetServices(void) const
{
	std::set<Service::Ptr> services;

	boost::mutex::scoped_lock lock(l_ServiceMutex);

	Service::WeakPtr wservice;
	BOOST_FOREACH(boost::tie(boost::tuples::ignore, wservice), l_ServicesCache[GetName()]) {
		Service::Ptr service = wservice.lock();

		if (!service)
			continue;

		services.insert(service);
	}

	return services;
}

int Host::GetTotalServices(void) const
{
	return GetServices().size();
}

void Host::InvalidateServicesCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_ServiceMutex);

		if (l_ServicesCacheNeedsUpdate)
			return; /* Someone else has already requested a refresh. */

		if (!l_ServicesCacheTimer) {
			l_ServicesCacheTimer = boost::make_shared<Timer>();
			l_ServicesCacheTimer->SetInterval(0.5);
			l_ServicesCacheTimer->OnTimerExpired.connect(boost::bind(&Host::RefreshServicesCache));
			l_ServicesCacheTimer->Start();
		}

		l_ServicesCacheNeedsUpdate = true;
	}
}

void Host::RefreshServicesCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_ServiceMutex);

		if (!l_ServicesCacheNeedsUpdate)
			return;

		l_ServicesCacheNeedsUpdate = false;
	}

	Log(LogDebug, "icinga", "Updating Host services cache.");

	std::map<String, std::map<String, Service::WeakPtr> > newServicesCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);

		Host::Ptr host = service->GetHost();

		if (!host)
			continue;

		// TODO: assert for duplicate short_names

		newServicesCache[host->GetName()][service->GetShortName()] = service;
	}

	boost::mutex::scoped_lock lock(l_ServiceMutex);
	l_ServicesCache.swap(newServicesCache);
}

Value Host::ValidateServiceDictionary(const String& location, const Dictionary::Ptr& attrs)
{
	ObjectLock olock(attrs);

	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), attrs) {
		std::vector<String> templates;

		if (!value.IsObjectType<Dictionary>())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Service description must be a dictionary."));

		Dictionary::Ptr serviceDesc = value;

		Array::Ptr templatesArray = serviceDesc->Get("templates");

		if (templatesArray) {
			ObjectLock tlock(templatesArray);

			BOOST_FOREACH(const Value& tmpl, templatesArray) {
				templates.push_back(tmpl);
			}
		}

		BOOST_FOREACH(const String& name, templates) {
			ConfigItem::Ptr item;

			ConfigCompilerContext *context = ConfigCompilerContext::GetContext();

			if (context)
				item = context->GetItem("Service", name);

			/* ignore already active objects while we're in the compiler
			 * context and linking to existing items is disabled. */
			if (!item && (!context || (context->GetFlags() & CompilerLinkExisting)))
				item = ConfigItem::GetObject("Service", name);

			if (!item) {
				ConfigCompilerContext::GetContext()->AddError(false, "Validation failed for " +
				    location + ": Template '" + name + "' not found.");
			}
		}
	}

	return Empty;
}

Service::Ptr Host::GetServiceByShortName(const Value& name) const
{
	if (name.IsScalar()) {
		{
			boost::mutex::scoped_lock lock(l_ServiceMutex);

			std::map<String, Service::WeakPtr>& services = l_ServicesCache[GetName()];
			std::map<String, Service::WeakPtr>::iterator it = services.find(name);

			if (it != services.end()) {
				Service::Ptr service = it->second.lock();
				ASSERT(service);
				return service;
			}
		}

		return Service::Ptr();
	} else if (name.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = name;
		String short_name;

		return Service::GetByNamePair(dict->Get("host"), dict->Get("service"));
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Host/Service name pair is invalid: " + name.Serialize()));
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

			if (!host)
				continue;

			parents.insert(host);
		}
	}

	return parents;
}

Service::Ptr Host::GetHostCheckService(void) const
{
	String host_check = GetHostCheck();

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

	Service::Ptr hc = GetHostCheckService();

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

	Service::Ptr hc = GetHostCheckService();

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

	Service::Ptr hc = GetHostCheckService();

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

double Host::GetLastStateChange(void) const
{
	Service::Ptr hc = GetHostCheckService();

	if (!hc)
		return IcingaApplication::GetInstance()->GetStartTime();

	return hc->GetLastStateChange();
}


double Host::GetLastHardStateChange(void) const
{
	Service::Ptr hc = GetHostCheckService();

	if (!hc)
		return IcingaApplication::GetInstance()->GetStartTime();

	return hc->GetLastHardStateChange();
}

StateType Host::GetLastStateType(void) const
{
	Service::Ptr hc = GetHostCheckService();

	if (!hc)
		return StateTypeHard;

	return hc->GetLastStateType();
}

StateType Host::GetStateType(void) const
{
	Service::Ptr hc = GetHostCheckService();

	if (!hc)
		return StateTypeHard;

	return hc->GetStateType();
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

bool Host::ResolveMacro(const String& macro, const Dictionary::Ptr&, String *result) const
{
	if (macro == "HOSTNAME") {
		*result = GetName();
		return true;
	}
	else if (macro == "HOSTDISPLAYNAME" || macro == "HOSTALIAS") {
		*result = GetDisplayName();
		return true;
	}

	Service::Ptr hc = GetHostCheckService();
	Dictionary::Ptr hccr;

	if (hc) {
		ServiceState state = hc->GetState();
		bool reachable = IsReachable();

		if (macro == "HOSTSTATE") {
			*result = Convert::ToString(CalculateState(state, reachable));
			return true;
		} else if (macro == "HOSTSTATEID") {
			*result = Convert::ToString(state);
			return true;
		} else if (macro == "HOSTSTATETYPE") {
			*result = Service::StateTypeToString(hc->GetStateType());
			return true;
		} else if (macro == "HOSTATTEMPT") {
			*result = Convert::ToString(hc->GetCurrentCheckAttempt());
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
			*result = hccr->Get("output");
			return true;
		} else if (macro == "HOSTPERFDATA") {
			*result = hccr->Get("performance_data_raw");
			return true;
		} else if (macro == "LASTHOSTCHECK") {
			*result = Convert::ToString((long)hccr->Get("schedule_start"));
			return true;
		}
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
