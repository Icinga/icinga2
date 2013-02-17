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

#include "i2-icinga.h"

using namespace icinga;

map<String, map<String, weak_ptr<Service> > > Host::m_ServicesCache;
bool Host::m_ServicesCacheValid = true;

REGISTER_SCRIPTFUNCTION("ValidateServiceDictionary", &Host::ValidateServiceDictionary);

static AttributeDescription hostAttributes[] = {
	{ "slave_services", Attribute_Transient }
};

REGISTER_TYPE(Host, hostAttributes);

Host::Host(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{ }

Host::~Host(void)
{
	HostGroup::InvalidateMembersCache();

	Dictionary::Ptr services = Get("slave_services");

	if (services) {
		ConfigItem::Ptr service;
		BOOST_FOREACH(tie(tuples::ignore, service), services) {
			service->Unregister();
		}
	}
}

String Host::GetDisplayName(void) const
{
	String value = Get("display_name");
	if (!value.IsEmpty())
		return value;
	else
		return GetName();
}

/**
 * @threadsafety Always.
 */
bool Host::Exists(const String& name)
{
	return (DynamicObject::GetObject("Host", name));
}

/**
 * @threadsafety Always.
 */
Host::Ptr Host::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Host", name);

	if (!configObject)
		BOOST_THROW_EXCEPTION(invalid_argument("Host '" + name + "' does not exist."));

	return dynamic_pointer_cast<Host>(configObject);
}

Dictionary::Ptr Host::GetGroups(void) const
{
	return Get("hostgroups");
}

Dictionary::Ptr Host::GetMacros(void) const
{
	return Get("macros");
}

Dictionary::Ptr Host::GetHostDependencies(void) const
{
	return Get("hostdependencies");
}

Dictionary::Ptr Host::GetServiceDependencies(void) const
{
	return Get("servicedependencies");
}

String Host::GetHostCheck(void) const
{
	return Get("hostcheck");
}

bool Host::IsReachable(void)
{
	BOOST_FOREACH(const Service::Ptr& service, GetParentServices()) {
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

	BOOST_FOREACH(const Host::Ptr& host, GetParentHosts()) {
		/* ignore hosts that are up */
		if (host->IsUp())
			continue;

		return false;
	}

	return true;
}

bool Host::IsInDowntime(void) const
{
	Service::Ptr service = GetHostCheckService();
	return (service || service->IsInDowntime());
}

bool Host::IsUp(void) const
{
	Service::Ptr service = GetHostCheckService();
	return (!service || service->GetState() == StateOK || service->GetState() == StateWarning);
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
	ConfigItem::Ptr item = ConfigItem::GetObject("Host", GetName());

	/* Don't create slave services unless we own this object
	 * and it's not a template. */
	if (!item || IsAbstract())
		return;

	Dictionary::Ptr oldServices = Get("slave_services");

	Dictionary::Ptr newServices;
	newServices = boost::make_shared<Dictionary>();

	Dictionary::Ptr serviceDescs = Get("services");

	if (serviceDescs) {
		String svcname;
		Value svcdesc;
		BOOST_FOREACH(tie(svcname, svcdesc), serviceDescs) {
			if (svcdesc.IsScalar())
				svcname = svcdesc;

			stringstream namebuf;
			namebuf << GetName() << "-" << svcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("Service");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, GetName());
			builder->AddExpression("display_name", OperatorSet, svcname);
			builder->AddExpression("short_name", OperatorSet, svcname);

			CopyServiceAttributes<false>(this, builder);

			if (svcdesc.IsScalar()) {
				builder->AddParent(svcdesc);
			} else if (svcdesc.IsObjectType<Dictionary>()) {
				Dictionary::Ptr service = svcdesc;

				Dictionary::Ptr templates = service->Get("templates");

				if (templates) {
					String tmpl;
					BOOST_FOREACH(tie(tuples::ignore, tmpl), templates) {
						builder->AddParent(tmpl);
					}
				} else {
					builder->AddParent(svcname);
				}

				CopyServiceAttributes<true>(service, builder);
			} else {
				BOOST_THROW_EXCEPTION(invalid_argument("Service description must be either a string or a dictionary."));
			}

			ConfigItem::Ptr serviceItem = builder->Compile();
			serviceItem->Commit();

			newServices->Set(name, serviceItem);
		}
	}

	if (oldServices) {
		ConfigItem::Ptr service;
		BOOST_FOREACH(tie(tuples::ignore, service), oldServices) {
			if (!service)
				continue;

			if (!newServices->Contains(service->GetName()))
				service->Unregister();
		}
	}

	Set("slave_services", newServices);
}

void Host::OnAttributeChanged(const String& name, const Value&)
{
	if (name == "hostgroups")
		HostGroup::InvalidateMembersCache();
	else if (name == "services")
		UpdateSlaveServices();
	else if (name == "notifications") {
		BOOST_FOREACH(const Service::Ptr& service, GetServices()) {
			service->UpdateSlaveNotifications();
		}
	}
}

set<Service::Ptr> Host::GetServices(void) const
{
	set<Service::Ptr> services;

	ValidateServicesCache();

	Service::WeakPtr wservice;
	BOOST_FOREACH(tie(tuples::ignore, wservice), m_ServicesCache[GetName()]) {
		Service::Ptr service = wservice.lock();

		if (!service)
			continue;

		services.insert(service);
	}

	return services;
}

void Host::InvalidateServicesCache(void)
{
	m_ServicesCacheValid = false;
	m_ServicesCache.clear();
}

void Host::ValidateServicesCache(void)
{
	if (m_ServicesCacheValid)
		return;

	m_ServicesCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);

		// TODO: assert for duplicate short_names

		m_ServicesCache[service->GetHost()->GetName()][service->GetShortName()] = service;
	}

	m_ServicesCacheValid = true;
}

void Host::ValidateServiceDictionary(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Location must be specified."));

	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Attribute dictionary must be specified."));

	String location = arguments[0];
	Dictionary::Ptr attrs = arguments[1];

	String key;
	Value value;
	BOOST_FOREACH(tie(key, value), attrs) {
		String name;

		if (value.IsScalar()) {
			name = value;
		} else if (value.IsObjectType<Dictionary>()) {
			Dictionary::Ptr serviceDesc = value;

			if (serviceDesc->Contains("service"))
				name = serviceDesc->Get("service");
			else
				name = key;
		} else {
			continue;
		}

		if (!ConfigItem::GetObject("Service", name)) {
			ConfigCompilerContext::GetContext()->AddError(false, "Validation failed for " +
			    location + ": Service '" + name + "' not found.");
		}
	}

	task->FinishResult(Empty);
}

Service::Ptr Host::GetServiceByShortName(const Value& name) const
{
	if (name.IsScalar()) {
		ValidateServicesCache();

		map<String, weak_ptr<Service> >& services = m_ServicesCache[GetName()];
		map<String, weak_ptr<Service> >::iterator it = services.find(name);

		if (it != services.end()) {
			Service::Ptr service = it->second.lock();
			assert(service);
			return service;
		}

		return Service::GetByName(name);
	} else if (name.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = name;
		return GetByName(dict->Get("host"))->GetServiceByShortName(dict->Get("service"));
	} else {
		BOOST_THROW_EXCEPTION(invalid_argument("Host/Service name pair is invalid."));
	}
}

set<Host::Ptr> Host::GetParentHosts(void) const
{
	set<Host::Ptr> parents;

	Dictionary::Ptr dependencies = GetHostDependencies();

	if (dependencies) {
		Value value;
		BOOST_FOREACH(tie(tuples::ignore, value), dependencies) {
			if (value == GetName())
				continue;

			parents.insert(Host::GetByName(value));
		}
	}

	return parents;
}

Service::Ptr Host::GetHostCheckService(void) const
{
	String hostcheck = GetHostCheck();

	if (hostcheck.IsEmpty())
		return Service::Ptr();

	Service::Ptr service = GetServiceByShortName(hostcheck);

	if (service->GetHost()->GetName() != GetName())
		BOOST_THROW_EXCEPTION(runtime_error("Hostcheck service refers to another host's service."));

	return service;
}

set<Service::Ptr> Host::GetParentServices(void) const
{
	set<Service::Ptr> parents;

	Dictionary::Ptr dependencies = GetServiceDependencies();

	if (dependencies) {
		Value value;
		BOOST_FOREACH(tie(tuples::ignore, value), dependencies) {
			parents.insert(GetServiceByShortName(value));
		}
	}

	return parents;
}

Dictionary::Ptr Host::CalculateDynamicMacros(void) const
{
	Dictionary::Ptr macros = boost::make_shared<Dictionary>();

	macros->Set("HOSTNAME", GetName());
	macros->Set("HOSTALIAS", GetName());
	macros->Set("HOSTDISPLAYNAME", GetDisplayName());
	macros->Set("HOSTSTATE", "DERP");

	Service::Ptr hostcheck = GetHostCheckService();

	if (hostcheck) {
		macros->Set("HOSTSTATEID", 99);
		macros->Set("HOSTSTATETYPE", Service::StateTypeToString(hostcheck->GetStateType()));
		macros->Set("HOSTATTEMPT", hostcheck->GetCurrentCheckAttempt());
		macros->Set("MAXHOSTATTEMPT", hostcheck->GetMaxCheckAttempts());
	}

	return macros;
}
