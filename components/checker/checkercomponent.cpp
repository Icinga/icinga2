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
		boost::bind(&CheckerComponent::AssignServiceRequestHandler, this, _2, _3));
	m_CheckerEndpoint->RegisterTopicHandler("checker::RevokeService",
		boost::bind(&CheckerComponent::RevokeServiceRequestHandler, this, _2, _3));
	m_CheckerEndpoint->RegisterTopicHandler("checker::ClearServices",
		boost::bind(&CheckerComponent::ClearServicesRequestHandler, this, _2, _3));
	m_CheckerEndpoint->RegisterPublication("checker::CheckResult");
	GetEndpointManager()->RegisterEndpoint(m_CheckerEndpoint);

	m_CheckTimer = boost::make_shared<Timer>();
	m_CheckTimer->SetInterval(10);
	m_CheckTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::CheckTimerHandler, this));
	m_CheckTimer->Start();

	CheckTask::RegisterType("nagios", NagiosCheckTask::CreateTask, NagiosCheckTask::FlushQueue, NagiosCheckTask::GetFinishedTasks);

	m_ResultTimer = boost::make_shared<Timer>();
	m_ResultTimer->SetInterval(5);
	m_ResultTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::ResultTimerHandler, this));
	m_ResultTimer->Start();
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

	Application::Log(LogDebug, "checker", "CheckTimerHandler entered.");

	long tasks = 0;

	while (!m_Services.empty()) {
		Service service = m_Services.top();

		if (service.GetNextCheck() > now)
			break;

		m_Services.pop();

//		Application::Log(LogInformation, "checker", "Executing service check for '" + service.GetName() + "'");

		CheckTask::Ptr task = CheckTask::CreateTask(service);
		task->Enqueue();

		tasks++;
	}

	Application::Log(LogDebug, "checker", "CheckTimerHandler: past loop.");

	CheckTask::FlushQueue();

	AdjustCheckTimer();

	stringstream msgbuf;
	msgbuf << "CheckTimerHandler: created " << tasks << " tasks";
	Application::Log(LogDebug, "checker", msgbuf.str());
}

void CheckerComponent::ResultTimerHandler(void)
{
	Application::Log(LogDebug, "checker", "ResultTimerHandler entered.");

	time_t now;
	time(&now);

	long results = 0;

	vector<CheckTask::Ptr> finishedTasks = CheckTask::GetFinishedTasks();

	for (vector<CheckTask::Ptr>::iterator it = finishedTasks.begin(); it != finishedTasks.end(); it++) {
		CheckTask::Ptr task = *it;

		Service service = task->GetService();

		CheckResult result = task->GetResult();
//		Application::Log(LogInformation, "checker", "Got result! Plugin output: " + result.Output);

		results++;

		service.SetNextCheck(now + service.GetCheckInterval());
		m_Services.push(service);
	}

	stringstream msgbuf;
	msgbuf << "ResultTimerHandler: " << results << " results";
	Application::Log(LogDebug, "checker", msgbuf.str());

	AdjustCheckTimer();
}

void CheckerComponent::AdjustCheckTimer(void)
{
	if (m_Services.empty())
		return;

	/* adjust next call time for the check timer */
	Service service = m_Services.top();
	m_CheckTimer->Reschedule(service.GetNextCheck());
}

void CheckerComponent::AssignServiceRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
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
	if (request.GetID(&id)) {
		ResponseMessage rm;
		rm.SetID(id);

		MessagePart result;
		rm.SetResult(result);
		GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, sender, rm);
	}
}

void CheckerComponent::RevokeServiceRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string name;
	if (!params.GetProperty("service", &name))
		return;

	vector<Service> services;

	while (!m_Services.empty()) {
		Service service = m_Services.top();

		if (service.GetName() == name)
			continue;

		// TODO: take care of services that are currently being checked

		services.push_back(service);
	}

	vector<Service>::const_iterator it;
	for (it = services.begin(); it != services.end(); it++)
		m_Services.push(*it);

	Application::Log(LogInformation, "checker", "Revoked delegation for service '" + name + "'");

	string id;
	if (request.GetID(&id)) {
		ResponseMessage rm;
		rm.SetID(id);

		MessagePart result;
		rm.SetResult(result);
		GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, sender, rm);
	}
}

void CheckerComponent::ClearServicesRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	Application::Log(LogInformation, "checker", "Clearing service delegations.");
	m_Services = ServiceQueue();

	string id;
	if (request.GetID(&id)) {
		ResponseMessage rm;
		rm.SetID(id);

		MessagePart result;
		rm.SetResult(result);
		GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, sender, rm);
	}
}

EXPORT_COMPONENT(checker, CheckerComponent);
