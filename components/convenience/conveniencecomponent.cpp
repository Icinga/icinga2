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
 * Returns the name of the component.
 *
 * @returns The name.
 */
string ConvenienceComponent::GetName(void) const
{
	return "convenience";
}

/**
 * Starts the component.
 */
void ConvenienceComponent::Start(void)
{
	ConfigItem::Set::Ptr itemSet = ConfigItem::GetAllObjects();
	itemSet->OnObjectAdded.connect(boost::bind(&ConvenienceComponent::HostAddedHandler, this, _2));
	itemSet->OnObjectCommitted.connect(boost::bind(&ConvenienceComponent::HostCommittedHandler, this, _2));
	itemSet->OnObjectRemoved.connect(boost::bind(&ConvenienceComponent::HostRemovedHandler, this, _2));
}

/**
 * Stops the component.
 */
void ConvenienceComponent::Stop(void)
{
}

void ConvenienceComponent::HostAddedHandler(const ConfigItem::Ptr& item)
{
	HostCommittedHandler(item);
}

void ConvenienceComponent::CopyServiceAttributes(const ConfigObject::Ptr& host, const Dictionary::Ptr& service, const ConfigItemBuilder::Ptr& builder)
{
	/* TODO: we only need to copy macros if this is an inline definition,
	 * i.e. host->GetProperties() != service, however for now we just
	 * copy them anyway. */
	Dictionary::Ptr macros;
	if (service->Get("macros", &macros))
		builder->AddExpression("macros", OperatorPlus, macros);

	long checkInterval;
	if (service->Get("check_interval", &checkInterval))
		builder->AddExpression("check_interval", OperatorSet, checkInterval);

	long retryInterval;
	if (service->Get("retry_interval", &retryInterval))
		builder->AddExpression("retry_interval", OperatorSet, retryInterval);

	Dictionary::Ptr sgroups;
	if (service->Get("servicegroups", &sgroups))
		builder->AddExpression("servicegroups", OperatorPlus, sgroups);

	Dictionary::Ptr checkers;
	if (service->Get("checkers", &checkers))
		builder->AddExpression("checkers", OperatorSet, checkers);

	Dictionary::Ptr dependencies;
	if (service->Get("dependencies", &dependencies))
		builder->AddExpression("dependencies", OperatorPlus,
		    Service::ResolveDependencies(host, dependencies));

	Dictionary::Ptr hostchecks;
	if (service->Get("hostchecks", &hostchecks))
		builder->AddExpression("dependencies", OperatorPlus,
		    Service::ResolveDependencies(host, hostchecks));
}

void ConvenienceComponent::HostCommittedHandler(const ConfigItem::Ptr& item)
{
	if (item->GetType() != "host")
		return;

	ConfigObject::Ptr host = ConfigObject::GetObject("host", item->GetName());

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
				throw invalid_argument("Service description must be either a string or a dictionary.");
			}

			ConfigItem::Ptr serviceItem = builder->Compile();
			serviceItem->Commit();

			newServices->Set(name, serviceItem);
		}
	}

	if (oldServices) {
		ConfigItem::Ptr service;
		BOOST_FOREACH(tie(tuples::ignore, service), oldServices) {
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
