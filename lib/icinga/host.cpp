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
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
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

/**
 * @threadsafety Always.
 */
Host::Ptr Host::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Host", name);

	return dynamic_pointer_cast<Host>(configObject);
}

Array::Ptr Host::GetGroups(void) const
{
	return m_HostGroups;;
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

template<bool copyServiceAttrs, typename TDict>
static void CopyServiceAttributes(TDict serviceDesc, const ConfigItemBuilder::Ptr& builder)
{
	/* TODO: we only need to copy macros if this is an inline definition,
	 * i.e. "typeid(serviceDesc)" != Service, however for now we just
	 * copy them anyway. */
	Value macros = serviceDesc->Get("macros");
	if (!macros.IsEmpty())
		builder->AddExpression("macros", OperatorPlus, macros);

	Value checkInterval = serviceDesc->Get("check_interval");
	if (!checkInterval.IsEmpty())
		builder->AddExpression("check_interval", OperatorSet, checkInterval);

	Value retryInterval = serviceDesc->Get("retry_interval");
	if (!retryInterval.IsEmpty())
		builder->AddExpression("retry_interval", OperatorSet, retryInterval);

	Value sgroups = serviceDesc->Get("servicegroups");
	if (!sgroups.IsEmpty())
		builder->AddExpression("servicegroups", OperatorPlus, sgroups);

	Value checkers = serviceDesc->Get("checkers");
	if (!checkers.IsEmpty())
		builder->AddExpression("checkers", OperatorSet, checkers);

	Value short_name = serviceDesc->Get("short_name");
	if (!short_name.IsEmpty())
		builder->AddExpression("short_name", OperatorSet, short_name);

	Value notification_interval = serviceDesc->Get("notification_interval");
	if (!notification_interval.IsEmpty())
		builder->AddExpression("notification_interval", OperatorSet, notification_interval);

	Value check_period = serviceDesc->Get("check_period");
	if (!check_period.IsEmpty())
		builder->AddExpression("check_period", OperatorSet, check_period);

	if (copyServiceAttrs) {
		Value servicedependencies = serviceDesc->Get("servicedependencies");
		if (!servicedependencies.IsEmpty())
			builder->AddExpression("servicedependencies", OperatorPlus, servicedependencies);

		Value hostdependencies = serviceDesc->Get("hostdependencies");
		if (!hostdependencies.IsEmpty())
			builder->AddExpression("hostdependencies", OperatorPlus, hostdependencies);
	}
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
			namebuf << GetName() << "-" << svcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("Service");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, GetName());
			builder->AddExpression("display_name", OperatorSet, svcname);
			builder->AddExpression("short_name", OperatorSet, svcname);

			CopyServiceAttributes<false>(this, builder);

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

			CopyServiceAttributes<true>(service, builder);

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

void Host::ValidateServiceDictionary(const ScriptTask::Ptr& task, const std::vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: Location must be specified."));

	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Missing argument: Attribute dictionary must be specified."));

	String location = arguments[0];
	Dictionary::Ptr attrs = arguments[1];
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

	task->FinishResult(Empty);
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

		ASSERT(dict->IsSealed());

		return Service::GetByNamePair(dict->Get("host"), dict->Get("service"));
	} else {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Host/Service name pair is invalid."));
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

StateType Host::GetLastStateType(void) const
{
	Service::Ptr hc = GetHostCheckService();

	if (!hc)
		return StateTypeHard;

	return hc->GetLastStateType();
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

Dictionary::Ptr Host::CalculateDynamicMacros(void) const
{
	ASSERT(!OwnsLock());

	Dictionary::Ptr macros = boost::make_shared<Dictionary>();

	{
		ObjectLock olock(this);

		macros->Set("HOSTNAME", GetName());
		macros->Set("HOSTDISPLAYNAME", GetDisplayName());
		macros->Set("HOSTALIAS", GetName());
	}

	Dictionary::Ptr cr;

	Service::Ptr hc = GetHostCheckService();

	if (hc) {
		ObjectLock olock(hc);

		ServiceState state = hc->GetState();
		bool reachable = IsReachable();

		macros->Set("HOSTSTATE", CalculateState(state, reachable));
		macros->Set("HOSTSTATEID", state);
		macros->Set("HOSTSTATETYPE", Service::StateTypeToString(hc->GetStateType()));
		macros->Set("HOSTATTEMPT", hc->GetCurrentCheckAttempt());
		macros->Set("MAXHOSTATTEMPT", hc->GetMaxCheckAttempts());

		macros->Set("LASTHOSTSTATE", StateToString(GetLastState()));
		macros->Set("LASTHOSTSTATEID", GetLastState());
		macros->Set("LASTHOSTSTATETYPE", Service::StateTypeToString(GetLastStateType()));
		macros->Set("LASTHOSTSTATECHANGE", (long)hc->GetLastStateChange());

		cr = hc->GetLastCheckResult();
	}

	if (cr) {
		macros->Set("HOSTLATENCY", Service::CalculateLatency(cr));
		macros->Set("HOSTEXECUTIONTIME", Service::CalculateExecutionTime(cr));

		macros->Set("HOSTOUTPUT", cr->Get("output"));
		macros->Set("HOSTPERFDATA", cr->Get("performance_data_raw"));

		macros->Set("LASTHOSTCHECK", (long)cr->Get("schedule_start"));
	}

	macros->Seal();

	return macros;
}
