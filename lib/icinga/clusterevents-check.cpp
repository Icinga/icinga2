/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "icinga/clusterevents.hpp"
#include "remote/apilistener.hpp"
#include "base/serializer.hpp"
#include "base/exception.hpp"
#include <boost/thread/once.hpp>
#include <thread>

using namespace icinga;

boost::mutex ClusterEvents::m_Mutex;
std::deque<std::function<void ()>> ClusterEvents::m_CheckRequestQueue;
bool ClusterEvents::m_CheckSchedulerRunning;
int ClusterEvents::m_ChecksExecutedDuringInterval;
int ClusterEvents::m_ChecksDroppedDuringInterval;
Timer::Ptr ClusterEvents::m_LogTimer;

void ClusterEvents::RemoteCheckThreadProc()
{
	Utility::SetThreadName("Remote Check Scheduler");

	boost::mutex::scoped_lock lock(m_Mutex);

	for(;;) {
		if (m_CheckRequestQueue.empty())
			break;

		lock.unlock();
		Checkable::AquirePendingCheckSlot(Application::GetMaxConcurrentChecks());
		lock.lock();

		auto callback = m_CheckRequestQueue.front();
		m_CheckRequestQueue.pop_front();
		m_ChecksExecutedDuringInterval++;
		lock.unlock();

		callback();
		Checkable::DecreasePendingChecks();

		lock.lock();
	}

	m_CheckSchedulerRunning = false;
}

void ClusterEvents::EnqueueCheck(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	static boost::once_flag once = BOOST_ONCE_INIT;

	boost::call_once(once, []() {
		m_LogTimer = new Timer();
		m_LogTimer->SetInterval(10);
		m_LogTimer->OnTimerExpired.connect(std::bind(ClusterEvents::LogRemoteCheckQueueInformation));
		m_LogTimer->Start();
	});

	boost::mutex::scoped_lock lock(m_Mutex);

	if (m_CheckRequestQueue.size() >= 25000) {
		m_ChecksDroppedDuringInterval++;
		return;
	}

	m_CheckRequestQueue.push_back(std::bind(ClusterEvents::ExecuteCheckFromQueue, origin, params));

	if (!m_CheckSchedulerRunning) {
		std::thread t(ClusterEvents::RemoteCheckThreadProc);
		t.detach();
		m_CheckSchedulerRunning = true;
	}
}

void ClusterEvents::ExecuteCheckFromQueue(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params) {

	Endpoint::Ptr sourceEndpoint = origin->FromClient->GetEndpoint();

	if (!sourceEndpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		Log(LogNotice, "ClusterEvents")
				<< "Discarding 'execute command' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return;
	}

	if (!listener->GetAcceptCommands()) {
		Log(LogWarning, "ApiListener")
				<< "Ignoring command. '" << listener->GetName() << "' does not accept commands.";

		Host::Ptr host = new Host();
		Dictionary::Ptr attrs = new Dictionary();

		attrs->Set("__name", params->Get("host"));
		attrs->Set("type", "Host");
		attrs->Set("enable_active_checks", false);

		Deserialize(host, attrs, false, FAConfig);

		if (params->Contains("service"))
			host->SetExtension("agent_service_name", params->Get("service"));

		CheckResult::Ptr cr = new CheckResult();
		cr->SetState(ServiceUnknown);
		cr->SetOutput("Endpoint '" + Endpoint::GetLocalEndpoint()->GetName() + "' does not accept commands.");
		Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
		listener->SyncSendMessage(sourceEndpoint, message);

		return;
	}

	/* use a virtual host object for executing the command */
	Host::Ptr host = new Host();
	Dictionary::Ptr attrs = new Dictionary();

	attrs->Set("__name", params->Get("host"));
	attrs->Set("type", "Host");

	Deserialize(host, attrs, false, FAConfig);

	if (params->Contains("service"))
		host->SetExtension("agent_service_name", params->Get("service"));

	String command = params->Get("command");
	String command_type = params->Get("command_type");

	if (command_type == "check_command") {
		if (!CheckCommand::GetByName(command)) {
			CheckResult::Ptr cr = new CheckResult();
			cr->SetState(ServiceUnknown);
			cr->SetOutput("Check command '" + command + "' does not exist.");
			Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
			listener->SyncSendMessage(sourceEndpoint, message);
			return;
		}
	} else if (command_type == "event_command") {
		if (!EventCommand::GetByName(command)) {
			Log(LogWarning, "ClusterEvents")
					<< "Event command '" << command << "' does not exist.";
			return;
		}
	} else
		return;

	attrs->Set(command_type, params->Get("command"));
	attrs->Set("command_endpoint", sourceEndpoint->GetName());

	Deserialize(host, attrs, false, FAConfig);

	host->SetExtension("agent_check", true);

	Dictionary::Ptr macros = params->Get("macros");

	if (command_type == "check_command") {
		try {
			host->ExecuteRemoteCheck(macros);
		} catch (const std::exception& ex) {
			CheckResult::Ptr cr = new CheckResult();
			cr->SetState(ServiceUnknown);

			String output = "Exception occurred while checking '" + host->GetName() + "': " + DiagnosticInformation(ex);
			cr->SetOutput(output);

			double now = Utility::GetTime();
			cr->SetScheduleStart(now);
			cr->SetScheduleEnd(now);
			cr->SetExecutionStart(now);
			cr->SetExecutionEnd(now);

			Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
			listener->SyncSendMessage(sourceEndpoint, message);

			Log(LogCritical, "checker", output);
		}
	} else if (command_type == "event_command") {
		host->ExecuteEventHandler(macros, true);
	}
}

int ClusterEvents::GetCheckRequestQueueSize()
{
	return m_CheckRequestQueue.size();
}

void ClusterEvents::LogRemoteCheckQueueInformation() {
	if (m_ChecksDroppedDuringInterval > 0) {
		Log(LogCritical, "ClusterEvents")
			<< "Remote check queue ran out of slots. "
			<< m_ChecksDroppedDuringInterval << " checks dropped.";
		m_ChecksDroppedDuringInterval = 0;
	}

	if (m_ChecksExecutedDuringInterval == 0)
		return;

	Log(LogInformation, "RemoteCheckQueue")
		<< "items: " << m_CheckRequestQueue.size()
		<< ", rate: " << m_ChecksExecutedDuringInterval / 10 << "/s "
		<< "(" << m_ChecksExecutedDuringInterval * 6 << "/min "
		<< m_ChecksExecutedDuringInterval * 6 * 5 << "/5min "
		<< m_ChecksExecutedDuringInterval * 6 * 15 << "/15min" << ");";

	m_ChecksExecutedDuringInterval = 0;
}