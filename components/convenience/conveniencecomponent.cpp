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

void ConvenienceComponent::HostCommittedHandler(const ConfigItem::Ptr& item)
{
	if (item->GetType() != "host")
		return;

	ConfigObject::Ptr host = ConfigObject::GetObject("host", item->GetName());
	Dictionary::Ptr oldServices;
	host->GetTag("convenience-services", &oldServices);

	Dictionary::Ptr newServices;
	newServices = boost::make_shared<Dictionary>();

	Dictionary::Ptr serviceDescs;
	host->GetProperty("services", &serviceDescs);

	if (serviceDescs) {
		Dictionary::Iterator it;
		for (it = serviceDescs->Begin(); it != serviceDescs->End(); it++) {
			Variant desc = it->second;
			ConfigItem::Ptr serviceItem;

			string name;

			if (desc.GetType() == VariantString) {
				stringstream namebuf;
				namebuf << item->GetName() << "-" << string(desc);
				name = namebuf.str();

				serviceItem = boost::make_shared<ConfigItem>("service", name, item->GetDebugInfo());
				serviceItem->AddParent(desc);

				ExpressionList::Ptr exprl = boost::make_shared<ExpressionList>();

				Expression localExpr("__local", OperatorSet, 1, item->GetDebugInfo());
				exprl->AddExpression(localExpr);

				Expression abstractExpr("__abstract", OperatorSet, 0, item->GetDebugInfo());
				exprl->AddExpression(abstractExpr);

				Expression typeExpr("__type", OperatorSet, "service", item->GetDebugInfo());
				exprl->AddExpression(typeExpr);

				Expression nameExpr("__name", OperatorSet, name, item->GetDebugInfo());
				exprl->AddExpression(nameExpr);

				Expression hostExpr("host_name", OperatorSet, item->GetName(), item->GetDebugInfo());
				exprl->AddExpression(hostExpr);

				Expression aliasExpr("alias", OperatorSet, string(desc), item->GetDebugInfo());
				exprl->AddExpression(aliasExpr);

				Dictionary::Ptr macros;
				if (host->GetProperty("macros", &macros)) {
					Expression macrosExpr("macros", OperatorPlus, macros, item->GetDebugInfo());
					exprl->AddExpression(macrosExpr);
				}

				long checkInterval;
				if (host->GetProperty("check_interval", &checkInterval)) {
					Expression checkExpr("check_interval", OperatorSet, checkInterval, item->GetDebugInfo());
					exprl->AddExpression(checkExpr);
				}

				long retryInterval;
				if (host->GetProperty("retry_interval", &retryInterval)) {
					Expression retryExpr("retry_interval", OperatorSet, retryInterval, item->GetDebugInfo());
					exprl->AddExpression(retryExpr);
				}

				Dictionary::Ptr sgroups;
				if (host->GetProperty("servicegroups", &sgroups)) {
					Expression sgroupsExpr("servicegroups", OperatorPlus, sgroups, item->GetDebugInfo());
					exprl->AddExpression(sgroupsExpr);
				}

				Dictionary::Ptr checkers;
				if (host->GetProperty("checkers", &checkers)) {
					Expression checkersExpr("checkers", OperatorSet, checkers, item->GetDebugInfo());
					exprl->AddExpression(checkersExpr);
				}

				serviceItem->SetExpressionList(exprl);
				ConfigObject::Ptr service = serviceItem->Commit();

				newServices->SetProperty(name, serviceItem);
			} else {
				throw runtime_error("Not supported.");
			}
		}
	}

	if (oldServices) {
		Dictionary::Iterator it;
		for (it = oldServices->Begin(); it != oldServices->End(); it++) {
			ConfigItem::Ptr service = static_pointer_cast<ConfigItem>(it->second.GetObject());

			if (!newServices->Contains(service->GetName()))
				service->Unregister();
		}
	}

	//host.GetConfigObject()->SetTag("convenience-services", newServices);
}

void ConvenienceComponent::HostRemovedHandler(const ConfigItem::Ptr& item)
{
	if (item->GetType() != "host")
		return;

	ConfigObject::Ptr host = item->GetConfigObject();

	Dictionary::Ptr services;
	host->GetTag("convenience-services", &services);

	if (!services)
		return;

	Dictionary::Iterator it;
	for (it = services->Begin(); it != services->End(); it++) {
		ConfigItem::Ptr service = static_pointer_cast<ConfigItem>(it->second.GetObject());
		service->Unregister();
	}
}

EXPORT_COMPONENT(convenience, ConvenienceComponent);
