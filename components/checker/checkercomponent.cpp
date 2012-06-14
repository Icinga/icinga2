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
	return "configcomponent";
}

void CheckerComponent::Start(void)
{
	m_CheckerEndpoint = make_shared<VirtualEndpoint>();
	m_CheckerEndpoint->RegisterTopicHandler("checker::AssignService",
		bind(&CheckerComponent::AssignServiceRequestHandler, this, _1));
	m_CheckerEndpoint->RegisterTopicHandler("checker::RevokeService",
		bind(&CheckerComponent::AssignServiceRequestHandler, this, _1));
	m_CheckerEndpoint->RegisterPublication("checker::CheckResult");
	GetEndpointManager()->RegisterEndpoint(m_CheckerEndpoint);

	RequestMessage rm;
	rm.SetMethod("checker::AssignService");
	GetEndpointManager()->SendAPIMessage(m_CheckerEndpoint, rm, bind(&CheckerComponent::TestResponseHandler, this, _1));

	// TODO: get rid of this
	ConfigObject::GetAllObjects()->OnObjectAdded.connect(bind(&CheckerComponent::NewServiceHandler, this, _1));

	m_CheckTimer = make_shared<Timer>();
	m_CheckTimer->SetInterval(10);
	m_CheckTimer->OnTimerExpired.connect(bind(&CheckerComponent::CheckTimerHandler, this, _1));
	m_CheckTimer->Start();

	CheckTask::RegisterType("nagios", NagiosCheckTask::CreateTask);

	ConfigObject::TMap::Range range = ConfigObject::GetObjects("service");

	for (ConfigObject::TMap::Iterator it = range.first; it != range.second; it++) {
		Service svc(it->second);
		CheckTask::Ptr ct = CheckTask::CreateTask(svc);
		CheckResult cr = ct->Execute();
	}
}

int CheckerComponent::TestResponseHandler(const NewResponseEventArgs& ea)
{
	return 0;
}

void CheckerComponent::Stop(void)
{

}

int CheckerComponent::NewServiceHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	if (ea.Target->GetType() == "service")
		m_Services.push(ea.Target);

	return 0;
}

int CheckerComponent::CheckTimerHandler(const TimerEventArgs& ea)
{
	time_t now;
	time(&now);

	if (m_Services.size() == 0)
		return 0;

	for (;;) {

		Service service = m_Services.top();

		if (service.GetNextCheck() > now)
			break;

		CheckTask::Ptr ct = CheckTask::CreateTask(service);
		CheckResult cr = ct->Execute();

		m_Services.pop();
		service.SetNextCheck(now + service.GetCheckInterval());
		m_Services.push(service);
	}

	/* adjust next call time for the check timer */
	Service service = m_Services.top();
	static_pointer_cast<Timer>(ea.Source)->SetInterval(service.GetNextCheck() - now);

	return 0;
}

int CheckerComponent::AssignServiceRequestHandler(const NewRequestEventArgs& nrea)
{
	string id;
	if (!nrea.Request.GetID(&id))
		return 0;

	ResponseMessage rm;
	rm.SetID(id);

	MessagePart result;
	rm.SetResult(result);
	GetEndpointManager()->SendUnicastMessage(m_CheckerEndpoint, nrea.Sender, rm);

	return 0;
}

int CheckerComponent::RevokeServiceRequestHandler(const NewRequestEventArgs& nrea)
{
	return 0;
}

EXPORT_COMPONENT(checker, CheckerComponent);
