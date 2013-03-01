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

boost::mutex Host::m_ServiceMutex;
map<String, map<String, Service::WeakPtr> > Host::m_ServicesCache;
bool Host::m_ServicesCacheValid = true;

REGISTER_SCRIPTFUNCTION("ValidateServiceDictionary", &Host::ValidateServiceDictionary);

REGISTER_TYPE(Host);

Host::Host(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
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
		BOOST_FOREACH(tie(tuples::ignore, service), m_SlaveServices) {
			service->Unregister();
		}
	}
}

void Host::OnRegistrationCompleted(void)
{
	DynamicObject::OnRegistrationCompleted();

	Host::InvalidateServicesCache();
	Host::UpdateSlaveServices(GetSelf());
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

Dictionary::Ptr Host::GetGroups(void) const
{
	return m_HostGroups;;
}

Dictionary::Ptr Host::GetMacros(void) const
{
	return m_Macros;
}

Dictionary::Ptr Host::GetHostDependencies(void) const
{
	return m_HostDependencies;;
}

Dictionary::Ptr Host::GetServiceDependencies(void) const
{
	return m_ServiceDependencies;
}

String Host::GetHostCheck(void) const
{
	return m_HostCheck;
}

bool Host::IsReachable(const Host::Ptr& self)
{
	set<Service::Ptr> parentServices = Host::GetParentServices(self);

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

	set<Host::Ptr> parentHosts = Host::GetParentHosts(self);

	BOOST_FOREACH(const Host::Ptr& host, parentHosts) {
		Service::Ptr hc = Host::GetHostCheckService(host);
		ObjectLock olock(hc);

		/* ignore hosts that are up */
		if (hc && hc->GetState() == StateOK)
			continue;

		return false;
	}

	return true;
}

template<bool copyServiceAttrs, typename TDict>
static void CopyServiceAttributes(TDict serviceDesc, const ConfigItemBuilder::Ptr& builder)
{
	ObjectLock olock(serviceDesc);

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

	if (copyServiceAttrs) {
		Value servicedependencies = serviceDesc->Get("servicedependencies");
		if (!servicedependencies.IsEmpty())
			builder->AddExpression("servicedependencies", OperatorPlus, servicedependencies);

		Value hostdependencies = serviceDesc->Get("hostdependencies");
		if (!hostdependencies.IsEmpty())
			builder->AddExpression("hostdependencies", OperatorPlus, hostdependencies);
	}
}

void Host::UpdateSlaveServices(const Host::Ptr& self)
{
	ConfigItem::Ptr item;
	Dictionary::Ptr oldServices, newServices, serviceDescs;
	String host_name;

	{
		ObjectLock olock(self);

		host_name = self->GetName();

		item = ConfigItem::GetObject("Host", host_name);

		/* Don't create slave services unless we own this object
		 * and it's not a template. */
		if (!item || self->IsAbstract())
			return;

		oldServices = self->m_SlaveServices;
		serviceDescs = self->Get("services");
	}

	newServices = boost::make_shared<Dictionary>();
	ObjectLock nlock(newServices);

	DebugInfo debug_info;

	{
		ObjectLock olock(item);
		debug_info = item->GetDebugInfo();
	}

	if (serviceDescs) {
		ObjectLock olock(serviceDescs);
		String svcname;
		Value svcdesc;
		BOOST_FOREACH(tie(svcname, svcdesc), serviceDescs) {
			if (svcdesc.IsScalar())
				svcname = svcdesc;

			stringstream namebuf;
			namebuf << host_name << "-" << svcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(debug_info);
			builder->SetType("Service");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, host_name);
			builder->AddExpression("display_name", OperatorSet, svcname);
			builder->AddExpression("short_name", OperatorSet, svcname);

			CopyServiceAttributes<false>(self, builder);

			if (svcdesc.IsScalar()) {
				builder->AddParent(svcdesc);
			} else if (svcdesc.IsObjectType<Dictionary>()) {
				Dictionary::Ptr service = svcdesc;

				Dictionary::Ptr templates;

				{
					ObjectLock olock(service);
					templates = service->Get("templates");
				}

				if (templates) {
					ObjectLock olock(templates);

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
			DynamicObject::Ptr dobj = ConfigItem::Commit(serviceItem);

			newServices->Set(name, serviceItem);
		}
	}

	if (oldServices) {
		ObjectLock olock(oldServices);

		ConfigItem::Ptr service;
		BOOST_FOREACH(tie(tuples::ignore, service), oldServices) {
			if (!service)
				continue;

			if (!newServices->Contains(service->GetName()))
				service->Unregister();
		}
	}

	newServices->Seal();

	ObjectLock olock(self);
	self->Set("slave_services", newServices);
}

void Host::OnAttributeChanged(const String& name, const Value&)
{
	if (name == "hostgroups")
		HostGroup::InvalidateMembersCache();
	else if (name == "services") {
		Host::Ptr self;

		{
			ObjectLock olock(this);
			self = GetSelf();
		}

		UpdateSlaveServices(self);
	} else if (name == "notifications") {
		set<Service::Ptr> services;

		{
			ObjectLock olock(this);
			services = GetServices();
		}

		BOOST_FOREACH(const Service::Ptr& service, services) {
			Service::UpdateSlaveNotifications(service);
		}
	}
}

set<Service::Ptr> Host::GetServices(void) const
{
	set<Service::Ptr> services;

	boost::mutex::scoped_lock lock(m_ServiceMutex);

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
	{
		boost::mutex::scoped_lock lock(m_ServiceMutex);

		if (m_ServicesCacheValid)
			Utility::QueueAsyncCallback(boost::bind(&Host::RefreshServicesCache));

		m_ServicesCacheValid = false;
	}
}

void Host::RefreshServicesCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_ServiceMutex);

		if (m_ServicesCacheValid)
			return;

		m_ServicesCacheValid = true;
	}

	map<String, map<String, Service::WeakPtr> > newServicesCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		const Service::Ptr& service = static_pointer_cast<Service>(object);

		Host::Ptr host;
		String short_name;

		{
			ObjectLock olock(service);
			host = service->GetHost();
			short_name = service->GetShortName();
		}

		if (!host)
			continue;

		String host_name;

		{
			ObjectLock olock(host);
			host_name = host->GetName();
		}

		// TODO: assert for duplicate short_names

		newServicesCache[host_name][short_name] = service;
	}

	boost::mutex::scoped_lock lock(m_ServiceMutex);
	m_ServicesCache.swap(newServicesCache);
}

void Host::ValidateServiceDictionary(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	if (arguments.size() < 1)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Location must be specified."));

	if (arguments.size() < 2)
		BOOST_THROW_EXCEPTION(invalid_argument("Missing argument: Attribute dictionary must be specified."));

	String location = arguments[0];
	Dictionary::Ptr attrs = arguments[1];
	ObjectLock olock(attrs);

	String key;
	Value value;
	BOOST_FOREACH(tie(key, value), attrs) {
		String name;

		if (value.IsScalar()) {
			name = value;
		} else if (value.IsObjectType<Dictionary>()) {
			Dictionary::Ptr serviceDesc = value;

			name = serviceDesc->Get("service");

			if (name.IsEmpty())
				name = key;
		} else {
			continue;
		}

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
			    location + ": Service '" + name + "' not found.");
		}
	}

	task->FinishResult(Empty);
}

Service::Ptr Host::GetServiceByShortName(const Host::Ptr& self, const Value& name)
{
	String host_name;

	{
		ObjectLock olock(self);
		host_name = self->GetName();
	}

	if (name.IsScalar()) {
		{
			boost::mutex::scoped_lock lock(m_ServiceMutex);

			map<String, Service::WeakPtr>& services = m_ServicesCache[host_name];
			map<String, Service::WeakPtr>::iterator it = services.find(name);

			if (it != services.end()) {
				Service::Ptr service = it->second.lock();
				assert(service);
				return service;
			}
		}

		return Service::Ptr();
	} else if (name.IsObjectType<Dictionary>()) {
		Dictionary::Ptr dict = name;
		String short_name;

		{
			ObjectLock olock(dict);
			host_name = dict->Get("host");
			short_name = dict->Get("service");
		}

		return Service::GetByNamePair(host_name, short_name);
	} else {
		BOOST_THROW_EXCEPTION(invalid_argument("Host/Service name pair is invalid."));
	}
}

set<Host::Ptr> Host::GetParentHosts(const Host::Ptr& self)
{
	set<Host::Ptr> parents;

	Dictionary::Ptr dependencies;
	String host_name;

	{
		ObjectLock olock(self);
		dependencies = self->GetHostDependencies();
		host_name = self->GetName();
	}

	if (dependencies) {
		Value value;
		BOOST_FOREACH(tie(tuples::ignore, value), dependencies) {
			if (value == host_name)
				continue;

			Host::Ptr host = GetByName(value);

			if (!host)
				continue;

			parents.insert(host);
		}
	}

	return parents;
}

Service::Ptr Host::GetHostCheckService(const Host::Ptr& self)
{
	String host_check;

	{
		ObjectLock olock(self);
		host_check = self->GetHostCheck();
	}

	if (host_check.IsEmpty())
		return Service::Ptr();

	return GetServiceByShortName(self, host_check);
}

set<Service::Ptr> Host::GetParentServices(const Host::Ptr& self)
{
	set<Service::Ptr> parents;

	Dictionary::Ptr dependencies;

	{
		ObjectLock olock(self);
		dependencies = self->GetServiceDependencies();
	}

	if (dependencies) {
		Value value;
		BOOST_FOREACH(tie(tuples::ignore, value), dependencies) {
			parents.insert(GetServiceByShortName(self, value));
		}
	}

	return parents;
}

Dictionary::Ptr Host::CalculateDynamicMacros(const Host::Ptr& self)
{
	Dictionary::Ptr macros = boost::make_shared<Dictionary>();
	ObjectLock mlock(macros);

	{
		ObjectLock olock(self);

		macros->Set("HOSTNAME", self->GetName());
		macros->Set("HOSTDISPLAYNAME", self->GetDisplayName());
		macros->Set("HOSTALIAS", self->GetName());
	}

	bool reachable = Host::IsReachable(self);

	Dictionary::Ptr cr;

	Service::Ptr hc = Host::GetHostCheckService(self);

	if (hc) {
		ObjectLock olock(hc);

		String state;
		int stateid;

		switch (hc->GetState()) {
			case StateOK:
			case StateWarning:
				state = "UP";
				stateid = 0;
				break;
			default:
				state = "DOWN";
				stateid = 1;
				break;
		}

		if (!reachable) {
			state = "UNREACHABLE";
			stateid = 2;
		}

		macros->Set("HOSTSTATE", state);
		macros->Set("HOSTSTATEID", stateid);
		macros->Set("HOSTSTATETYPE", Service::StateTypeToString(hc->GetStateType()));
		macros->Set("HOSTATTEMPT", hc->GetCurrentCheckAttempt());
		macros->Set("MAXHOSTATTEMPT", hc->GetMaxCheckAttempts());

		cr = hc->GetLastCheckResult();
	}

	if (cr) {
		macros->Set("HOSTLATENCY", Service::CalculateLatency(cr));
		macros->Set("HOSTEXECUTIONTIME", Service::CalculateExecutionTime(cr));

		ObjectLock olock(cr);

		macros->Set("HOSTOUTPUT", cr->Get("output"));
		macros->Set("HOSTPERFDATA", cr->Get("performance_data_raw"));
	}

	macros->Seal();

	return macros;
}
