/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "agent/agentchecktask.h"
#include "agent/agentlistener.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/application.h"
#include "base/objectlock.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/initialize.h"
#include "base/scriptfunction.h"
#include "base/dynamictype.h"

using namespace icinga;

boost::mutex l_Mutex;
std::map<Checkable::Ptr, double> l_PendingChecks;
Timer::Ptr l_AgentTimer;

INITIALIZE_ONCE(&AgentCheckTask::StaticInitialize);
REGISTER_SCRIPTFUNCTION(AgentCheck, &AgentCheckTask::ScriptFunc);

void AgentCheckTask::StaticInitialize(void)
{
	l_AgentTimer = make_shared<Timer>();
	l_AgentTimer->OnTimerExpired.connect(boost::bind(&AgentCheckTask::AgentTimerHandler));
	l_AgentTimer->SetInterval(60);
	l_AgentTimer->Start();
}

void AgentCheckTask::AgentTimerHandler(void)
{
	boost::mutex::scoped_lock lock(l_Mutex);

	std::map<Checkable::Ptr, double> newmap;
	std::pair<Checkable::Ptr, double> kv;
	
	double now = Utility::GetTime();

	BOOST_FOREACH(kv, l_PendingChecks) {
		if (kv.second < now - 60 && kv.first->IsCheckPending()) {
			CheckResult::Ptr cr = make_shared<CheckResult>();
			cr->SetOutput("Agent isn't responding.");
			cr->SetState(ServiceCritical);
			kv.first->ProcessCheckResult(cr);
		} else {
			newmap.insert(kv);
		}
	}
	
	l_PendingChecks.swap(newmap);
}

void AgentCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("command", checkable->GetCheckCommand()));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	String agent_identity = MacroProcessor::ResolveMacros("$agent_identity$", resolvers, checkable->GetLastCheckResult());
	String agent_host = MacroProcessor::ResolveMacros("$agent_host$", resolvers, checkable->GetLastCheckResult());
	String agent_service = MacroProcessor::ResolveMacros("$agent_service$", resolvers, checkable->GetLastCheckResult());

	if (agent_identity.IsEmpty() || agent_host.IsEmpty()) {
		Log(LogWarning, "agent", "'agent_name' and 'agent_host' must be set for agent checks.");
		return;
	}

	String agent_peer_host = MacroProcessor::ResolveMacros("$agent_peer_host$", resolvers, checkable->GetLastCheckResult());
	String agent_peer_port = MacroProcessor::ResolveMacros("$agent_peer_port$", resolvers, checkable->GetLastCheckResult());
	
	double now = Utility::GetTime();
	
	BOOST_FOREACH(const AgentListener::Ptr& al, DynamicType::GetObjects<AgentListener>()) {
		double seen = al->GetAgentSeen(agent_identity);

		if (seen < now - 300)
			continue;

		CheckResult::Ptr cr = al->GetCheckResult(agent_identity, agent_host, agent_service);

		if (cr) {
			checkable->ProcessCheckResult(cr);
			return;
		}
	}
	
	{
		boost::mutex::scoped_lock lock(l_Mutex);
		l_PendingChecks[checkable] = now;
	}

	BOOST_FOREACH(const AgentListener::Ptr& al, DynamicType::GetObjects<AgentListener>()) {
		if (!agent_peer_host.IsEmpty() && !agent_peer_port.IsEmpty())
			al->AddConnection(agent_peer_host, agent_peer_port);
	}
}
