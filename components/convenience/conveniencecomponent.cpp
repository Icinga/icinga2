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

#include "i2-convenience.h"

using namespace icinga;

/**
 * Starts the component.
 */
void ConvenienceComponent::Start(void)
{
	ConfigItem::OnCommitted.connect(boost::bind(&ConvenienceComponent::HostCommittedHandler, this, _1));
	ConfigItem::OnRemoved.connect(boost::bind(&ConvenienceComponent::HostRemovedHandler, this, _1));
}

void ConvenienceComponent::CopyServiceAttributes(const Host::Ptr& host, const Dictionary::Ptr& serviceDesc, const ConfigItemBuilder::Ptr& builder)
{
	/* TODO: we only need to copy macros if this is an inline definition,
	 * i.e. host->GetProperties() != service, however for now we just
	 * copy them anyway. */
	Dictionary::Ptr macros;
	if (serviceDesc->Get("macros", &macros))
		builder->AddExpression("macros", OperatorPlus, macros);

	long checkInterval;
	if (serviceDesc->Get("check_interval", &checkInterval))
		builder->AddExpression("check_interval", OperatorSet, checkInterval);

	long retryInterval;
	if (serviceDesc->Get("retry_interval", &retryInterval))
		builder->AddExpression("retry_interval", OperatorSet, retryInterval);

	Dictionary::Ptr sgroups;
	if (serviceDesc->Get("servicegroups", &sgroups))
		builder->AddExpression("servicegroups", OperatorPlus, sgroups);

	Dictionary::Ptr checkers;
	if (serviceDesc->Get("checkers", &checkers))
		builder->AddExpression("checkers", OperatorSet, checkers);

	Dictionary::Ptr dependencies;
	if (serviceDesc->Get("dependencies", &dependencies))
		builder->AddExpression("dependencies", OperatorPlus,
		    Service::ResolveDependencies(host, dependencies));

	Dictionary::Ptr hostchecks;
	if (serviceDesc->Get("hostchecks", &hostchecks))
		builder->AddExpression("dependencies", OperatorPlus,
		    Service::ResolveDependencies(host, hostchecks));
}

void ConvenienceComponent::HostCommittedHandler(const ConfigItem::Ptr& item)
{
	if (item->GetType() != "host")
		return;

	Host::Ptr host = Host::GetByName(item->GetName());

	/* ignore abstract host objects */
	if (!host)
		return;

	Dictionary::Ptr oldServices;
	host->GetTag("convenience-services", &oldServices);

	Dictionary::Ptr newServices;
	newServices = boost::make_shared<Dictionary>();

	Dictionary::Ptr serviceDescs;
	host->GetProperty("services", &serviceDescs);

	if (serviceDescs) {
		string svcname;
		Variant svcdesc;
		BOOST_FOREACH(tie(svcname, svcdesc), serviceDescs) {
			stringstream namebuf;
			namebuf << item->GetName() << "-" << svcname;
			string name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("service");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, item->GetName());
			builder->AddExpression("alias", OperatorSet, svcname);

			CopyServiceAttributes(host, host->GetProperties(), builder);

			if (svcdesc.IsScalar()) {
				builder->AddParent(svcdesc);
			} else if (svcdesc.IsObjectType<Dictionary>()) {
				Dictionary::Ptr service = svcdesc;

				string parent;
				if (!service->Get("service", &parent))
					parent = svcname;

				builder->AddParent(parent);

				CopyServiceAttributes(host, service, builder);
			} else {
				throw_exception(invalid_argument("Service description must be either a string or a dictionary."));
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

	host->SetTag("convenience-services", newServices);
}

void ConvenienceComponent::HostRemovedHandler(const ConfigItem::Ptr& item)
{
	if (item->GetType() != "host")
		return;

	ConfigObject::Ptr host = item->GetConfigObject();

	if (!host)
		return;

	Dictionary::Ptr services;
	host->GetTag("convenience-services", &services);

	if (!services)
		return;

	ConfigItem::Ptr service;
	BOOST_FOREACH(tie(tuples::ignore, service), services) {
		service->Unregister();
	}
}

EXPORT_COMPONENT(convenience, ConvenienceComponent);
