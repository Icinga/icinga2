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

REGISTER_SCRIPTFUNCTION("native::ValidateServiceDictionary", &Host::ValidateServiceDictionary);

static AttributeDescription hostAttributes[] = {
	{ "acknowledgement", Attribute_Replicated },
	{ "acknowledgement_expiry", Attribute_Replicated },
	{ "downtimes", Attribute_Replicated },
	{ "comments", Attribute_Replicated },
	{ "convenience_services", Attribute_Transient }
};

REGISTER_TYPE(Host, hostAttributes);

Host::Host(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{ }

void Host::OnInitCompleted(void)
{
	HostGroup::InvalidateMembersCache();
	DowntimeProcessor::InvalidateDowntimeCache();

	Event::Post(boost::bind(&Host::UpdateSlaveServices, this));
}

Host::~Host(void)
{
	HostGroup::InvalidateMembersCache();
	DowntimeProcessor::InvalidateDowntimeCache();

	Dictionary::Ptr services = Get("convenience_services");

	if (services) {
		ConfigItem::Ptr service;
		BOOST_FOREACH(tie(tuples::ignore, service), services) {
			service->Unregister();
		}
	}
}

String Host::GetAlias(void) const
{
	String value = Get("alias");
	if (!value.IsEmpty())
		return value;
	else
		return GetName();
}

bool Host::Exists(const String& name)
{
	return (DynamicObject::GetObject("Host", name));
}

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

Dictionary::Ptr Host::GetDowntimes(void) const
{
	DowntimeProcessor::ValidateDowntimeCache();

	return Get("downtimes");
}

Dictionary::Ptr Host::GetComments(void) const
{
	CommentProcessor::ValidateCommentCache();

	return Get("comments");
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
	Dictionary::Ptr downtimes = GetDowntimes();

	if (!downtimes)
		return false;

	Dictionary::Ptr downtime;
	BOOST_FOREACH(tie(tuples::ignore, downtime), downtimes) {
		if (DowntimeProcessor::IsDowntimeActive(downtime))
			return true;
	}

	return false;
}

bool Host::IsUp(void) const
{
	Service::Ptr service = GetHostCheckService();
	return (!service || service->GetState() == StateOK || service->GetState() == StateWarning);
}

template<typename TDict>
static void CopyServiceAttributes(TDict serviceDesc, const ConfigItemBuilder::Ptr& builder)
{
	/* TODO: we only need to copy macros if this is an inline definition,
	 * i.e. host->GetProperties() != service, however for now we just
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
}

void Host::UpdateSlaveServices(void)
{
	ConfigItem::Ptr item = ConfigItem::GetObject("Host", GetName());

	/* Don't create slave services unless we own this object
	 * and it's not a template. */
	if (!item || IsAbstract())
		return;

	Dictionary::Ptr oldServices = Get("convenience_services");

	Dictionary::Ptr newServices;
	newServices = boost::make_shared<Dictionary>();

	Dictionary::Ptr serviceDescs = Get("services");

	if (serviceDescs) {
		String svcname;
		Value svcdesc;
		BOOST_FOREACH(tie(svcname, svcdesc), serviceDescs) {
			stringstream namebuf;
			namebuf << GetName() << "-" << svcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("Service");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, item->GetName());
			builder->AddExpression("alias", OperatorSet, svcname);
			builder->AddExpression("short_name", OperatorSet, svcname);

			CopyServiceAttributes(this, builder);

			if (svcdesc.IsScalar()) {
				builder->AddParent(svcdesc);
			} else if (svcdesc.IsObjectType<Dictionary>()) {
				Dictionary::Ptr service = svcdesc;

				String parent = service->Get("service");
				if (parent.IsEmpty())
					parent = svcname;

				builder->AddParent(parent);

				CopyServiceAttributes(service, builder);
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

	Set("convenience_services", newServices);
}

void Host::OnAttributeChanged(const String& name, const Value&)
{
	if (name == "hostgroups")
		HostGroup::InvalidateMembersCache();
	else if (name == "downtimes")
		DowntimeProcessor::InvalidateDowntimeCache();
	else if (name == "services")
		UpdateSlaveServices();
}

set<Service::Ptr> Host::GetServices(void) const
{
	set<Service::Ptr> services;

	ValidateServicesCache();

	String key;
	Service::WeakPtr wservice;
	BOOST_FOREACH(tie(key, wservice), m_ServicesCache[GetName()]) {
		Service::Ptr service = wservice.lock();

		if (!service)
			continue;

		services.insert(service);
	}

	return services;
}

AcknowledgementType Host::GetAcknowledgement(void)
{
	Value value = Get("acknowledgement");

	if (value.IsEmpty())
		return AcknowledgementNone;

	int ivalue = static_cast<int>(value);
	AcknowledgementType avalue = static_cast<AcknowledgementType>(ivalue);

	if (avalue != AcknowledgementNone) {
		double expiry = GetAcknowledgementExpiry();

		if (expiry != 0 && expiry < Utility::GetTime()) {
			avalue = AcknowledgementNone;
			SetAcknowledgement(avalue);
			SetAcknowledgementExpiry(0);
		}
	}

	return avalue;
}

void Host::SetAcknowledgement(AcknowledgementType acknowledgement)
{
	Set("acknowledgement", static_cast<long>(acknowledgement));
}

double Host::GetAcknowledgementExpiry(void) const
{
	Value value = Get("acknowledgement_expiry");

	if (value.IsEmpty())
		return 0;

	return static_cast<double>(value);
}

void Host::SetAcknowledgementExpiry(double timestamp)
{
	Set("acknowledgement_expiry", timestamp);
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
		String key;
		BOOST_FOREACH(tie(key, tuples::ignore), dependencies) {
			if (key == GetName())
				continue;

			parents.insert(Host::GetByName(key));
		}
	}

	return parents;
}

Service::Ptr Host::GetHostCheckService(void) const
{
	String hostcheck = GetHostCheck();

	if (hostcheck.IsEmpty())
		return Service::Ptr();

	return GetServiceByShortName(hostcheck);
}

set<Service::Ptr> Host::GetParentServices(void) const
{
	set<Service::Ptr> parents;

	Dictionary::Ptr dependencies = GetServiceDependencies();

	if (dependencies) {
		String key;
		Value value;
		BOOST_FOREACH(tie(key, value), dependencies) {
			parents.insert(GetServiceByShortName(value));
		}
	}

	return parents;
}
