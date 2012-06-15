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

#include "i2-checker.h"

using namespace icinga;

string CheckerComponent::GetName(void) const
{
	return "checker";
}

void CheckerComponent::Start(void)
{
	m_CheckerEndpoint = boost::make_shared<VirtualEndpoint>();
	m_CheckerEndpoint->RegisterTopicHandler("checker::AssignService",
		boost::bind(&CheckerComponent::AssignServiceRequestHandler, this, _1));
	m_CheckerEndpoint->RegisterTopicHandler("checker::RevokeService",
		boost::bind(&CheckerComponent::RevokeServiceRequestHandler, this, _1));
	m_CheckerEndpoint->RegisterTopicHandler("checker::ClearServices",
		boost::bind(&CheckerComponent::ClearServicesRequestHandler, this, _1));
	m_CheckerEndpoint->RegisterPublication("checker::CheckResult");
	GetEndpointManager()->RegisterEndpoint(m_CheckerEndpoint);

	m_CheckTimer = boost::make_shared<Timer>();
	m_CheckTimer->SetInterval(10);
	m_CheckTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::CheckTimerHandler, this));
	m_CheckTimer->Start();

	CheckTask::RegisterType("nagios", NagiosCheckTask::CreateTask);

	ConfigObject::TMap::Range range = ConfigObject::GetObjects("service");

	for (ConfigObject::TMap::Iterator it = range.first; it != range.second; it++) {
		Service svc(it->second);
		CheckTask::Ptr ct = CheckTask::CreateTask(svc);
		CheckResult cr = ct->Execute();
	}
}

void CheckerComponent::Stop(void)
{
	EndpointManager::Ptr mgr = GetEndpointManager();

	if (mgr)
		mgr->UnregisterEndpoint(m_CheckerEndpoint);
}

void CheckerComponent::CheckTimerHandler(void)
{
	time_t now;
	time(&now);

	if (m_Services.size() == 0)
		return;

	for (;;) {
		Service service = m_Services.top();

		if (service.GetNextCheck() > now)
			break;

		CheckTask::Ptr ct = CheckTask::CreateTask(service);
		Application::Log(LogInformation, "checker", "Executing service check for '" + service.GetName() + "'");
		CheckResult cr = ct->Execute();

		m_Services.pop();
		service.SetNextCheck(now + service.GetCheckInterval());
		m_Services.push(service);
	}

	/* adjust next call time for the check timer */
	Service service = m_Services.top();
	m_CheckTimer->SetInterval(service.GetNextCheck() - now);
}

void CheckerComponent::AssignServiceRequestHandler(const NewRequestEventArgs& nrea)
{
	MessagePart params;
	if (!nrea.Request.GetParams(&params))
		return;

	MessagePart serviceMsg;
	if (!params.GetProperty("service", &serviceMsg))
		return;

	ConfigObject::Ptr object = boost::make_shared<ConfigObject>(serviceMsg.GetDictionary());
	Service service(object);
	m_Services.push(service);

	Application::Log(LogInformation, "checker", "Accepted delegation for service '" + service.GetName() + "'");

	/* force a service check */
	m_CheckTimer->Reschedule(0);

	string id;
	if (nrea.Request.GetID(&id)) {
		ResponseMessage rm;
		rm.SetID(id);

		MessagePart result;
		rm.SetResult(result);
		GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, nrea.Sender, rm);
	}
}

void CheckerComponent::RevokeServiceRequestHandler(const NewRequestEventArgs& nrea)
{
	MessagePart params;
	if (!nrea.Request.GetParams(&params))
		return;

	string name;
	if (!params.GetProperty("service", &name))
		return;

	vector<Service> services;

	while (!m_Services.empty()) {
		Service service = m_Services.top();

		if (service.GetName() == name)
			continue;

		services.push_back(service);
	}

	vector<Service>::const_iterator it;
	for (it = services.begin(); it != services.end(); it++)
		m_Services.push(*it);

	Application::Log(LogInformation, "checker", "Revoked delegation for service '" + name + "'");

	string id;
	if (nrea.Request.GetID(&id)) {
		ResponseMessage rm;
		rm.SetID(id);

		MessagePart result;
		rm.SetResult(result);
		GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, nrea.Sender, rm);
	}
}

void CheckerComponent::ClearServicesRequestHandler(const NewRequestEventArgs& nrea)
{
	Application::Log(LogInformation, "checker", "Clearing service delegations.");
	m_Services = ServiceQueue();

	string id;
	if (nrea.Request.GetID(&id)) {
		ResponseMessage rm;
		rm.SetID(id);

		MessagePart result;
		rm.SetResult(result);
		GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, nrea.Sender, rm);
	}
}

EXPORT_COMPONENT(checker, CheckerComponent);
