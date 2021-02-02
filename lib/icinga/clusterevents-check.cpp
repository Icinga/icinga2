/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/clusterevents.hpp"
#include "icinga/icingaapplication.hpp"
#include "remote/apilistener.hpp"
#include "base/configuration.hpp"
#include "base/defer.hpp"
#include "base/serializer.hpp"
#include "base/exception.hpp"
#include <boost/thread/once.hpp>
#include <thread>

using namespace icinga;

std::mutex ClusterEvents::m_Mutex;
std::deque<std::function<void ()>> ClusterEvents::m_CheckRequestQueue;
bool ClusterEvents::m_CheckSchedulerRunning;
int ClusterEvents::m_ChecksExecutedDuringInterval;
int ClusterEvents::m_ChecksDroppedDuringInterval;
Timer::Ptr ClusterEvents::m_LogTimer;

void ClusterEvents::RemoteCheckThreadProc()
{
	Utility::SetThreadName("Remote Check Scheduler");

	int maxConcurrentChecks = IcingaApplication::GetInstance()->GetMaxConcurrentChecks();

	std::unique_lock<std::mutex> lock(m_Mutex);

	for(;;) {
		if (m_CheckRequestQueue.empty())
			break;

		lock.unlock();
		Checkable::AquirePendingCheckSlot(maxConcurrentChecks);
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

	std::unique_lock<std::mutex> lock(m_Mutex);

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

static void SendEventExecutedCommand(const Dictionary::Ptr& params, long exitStatus, const String& output,
	double start, double end, const ApiListener::Ptr& listener, const MessageOrigin::Ptr& origin,
	const Endpoint::Ptr& sourceEndpoint)
{
	Dictionary::Ptr executedParams = new Dictionary();
	executedParams->Set("execution", params->Get("source"));
	executedParams->Set("host", params->Get("host"));

	if (params->Contains("service"))
		executedParams->Set("service", params->Get("service"));

	executedParams->Set("exit", exitStatus);
	executedParams->Set("output", output);
	executedParams->Set("start", start);
	executedParams->Set("end", end);

	if (origin->IsLocal()) {
		ClusterEvents::ExecutedCommandAPIHandler(origin, executedParams);
	} else {
		Dictionary::Ptr executedMessage = new Dictionary();
		executedMessage->Set("jsonrpc", "2.0");
		executedMessage->Set("method", "event::ExecutedCommand");
		executedMessage->Set("params", executedParams);

		listener->SyncSendMessage(sourceEndpoint, executedMessage);
	}
}

void ClusterEvents::ExecuteCheckFromQueue(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params) {

	Endpoint::Ptr sourceEndpoint;

	if (origin->FromClient) {
		sourceEndpoint = origin->FromClient->GetEndpoint();
	} else if (origin->IsLocal()){
		sourceEndpoint = Endpoint::GetLocalEndpoint();
	}

	if (!sourceEndpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		Log(LogNotice, "ClusterEvents")
			<< "Discarding 'execute command' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener") << "No instance available.";
		return;
	}

	Defer resetExecuteCommandProcessFinishedHandler ([]() {
		Checkable::ExecuteCommandProcessFinishedHandler = nullptr;
	});

	if (params->Contains("source")) {
		String uuid = params->Get("source");

		String checkableName = params->Get("host");

		if (params->Contains("service"))
			checkableName += "!" + params->Get("service");

		/* Check deadline */
		double deadline = params->Get("deadline");

		if (Utility::GetTime() > deadline) {
			Log(LogNotice, "ApiListener")
				<< "Discarding 'ExecuteCheckFromQueue' event for checkable '" << checkableName
				<< "' from '" << origin->FromClient->GetIdentity() << "': Deadline has expired.";
			return;
		}

		Checkable::ExecuteCommandProcessFinishedHandler = [checkableName, listener, sourceEndpoint, origin, params] (const Value& commandLine, const ProcessResult& pr) {
			if (params->Get("command_type") == "check_command") {
				Checkable::CurrentConcurrentChecks.fetch_sub(1);
				Checkable::DecreasePendingChecks();
			}

			if (pr.ExitStatus > 3) {
				Process::Arguments parguments = Process::PrepareCommand(commandLine);
				Log(LogWarning, "ApiListener")
					<< "Command for object '" << checkableName << "' (PID: " << pr.PID
					<< ", arguments: " << Process::PrettyPrintArguments(parguments) << ") terminated with exit code "
					<< pr.ExitStatus << ", output: " << pr.Output;
			}

			SendEventExecutedCommand(params, pr.ExitStatus, pr.Output, pr.ExecutionStart, pr.ExecutionEnd, listener,
									origin, sourceEndpoint);
		};
	}

	if (!listener->GetAcceptCommands() && !origin->IsLocal()) {
		Log(LogWarning, "ApiListener")
			<< "Ignoring command. '" << listener->GetName() << "' does not accept commands.";

		String output = "Endpoint '" + Endpoint::GetLocalEndpoint()->GetName() + "' does not accept commands.";

		if (params->Contains("source")) {
			double now = Utility::GetTime();
			SendEventExecutedCommand(params, 126, output, now, now, listener, origin, sourceEndpoint);
		} else {
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
			cr->SetOutput(output);

			Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
			listener->SyncSendMessage(sourceEndpoint, message);
		}

		return;
	}

	/* use a virtual host object for executing the command */
	Host::Ptr host = new Host();
	Dictionary::Ptr attrs = new Dictionary();

	attrs->Set("__name", params->Get("host"));
	attrs->Set("type", "Host");

	/*
	 * Override the check timeout if the parent caller provided the value. Compatible with older versions not
	 * passing this inside the cluster message.
	 * This happens with host/service command_endpoint agents and the 'check_timeout' attribute being specified.
	 */
	if (params->Contains("check_timeout"))
		attrs->Set("check_timeout", params->Get("check_timeout"));

	Deserialize(host, attrs, false, FAConfig);

	if (params->Contains("service"))
		host->SetExtension("agent_service_name", params->Get("service"));

	String command = params->Get("command");
	String command_type = params->Get("command_type");

	if (command_type == "check_command") {
		if (!CheckCommand::GetByName(command)) {
			ServiceState state = ServiceUnknown;
			String output = "Check command '" + command + "' does not exist.";
			double now = Utility::GetTime();
			
			if (params->Contains("source")) {
				SendEventExecutedCommand(params, state, output, now, now, listener, origin, sourceEndpoint);
			} else {
				CheckResult::Ptr cr = new CheckResult();
				cr->SetState(state);
				cr->SetOutput(output);
				Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
				listener->SyncSendMessage(sourceEndpoint, message);
			}

			return;
		}
	} else if (command_type == "event_command") {
		if (!EventCommand::GetByName(command)) {
			String output = "Event command '" + command + "' does not exist.";
			Log(LogWarning, "ClusterEvents") << output;

			if (params->Contains("source")) {
				double now = Utility::GetTime();
				SendEventExecutedCommand(params, ServiceUnknown, output, now, now, listener, origin, sourceEndpoint);
			}

			return;
		}
	} else if (command_type == "notification_command") {
		if (!NotificationCommand::GetByName(command)) {
			String output = "Notification command '" + command + "' does not exist.";
			Log(LogWarning, "ClusterEvents") << output;

			if (params->Contains("source")) {
				double now = Utility::GetTime();
				SendEventExecutedCommand(params, ServiceUnknown, output, now, now, listener, origin, sourceEndpoint);
			}

			return;
		}
	}

	attrs->Set(command_type, params->Get("command"));
	attrs->Set("command_endpoint", sourceEndpoint->GetName());

	Deserialize(host, attrs, false, FAConfig);

	host->SetExtension("agent_check", true);

	Dictionary::Ptr macros = params->Get("macros");

	if (command_type == "check_command") {
		try {
			host->ExecuteRemoteCheck(macros);
		} catch (const std::exception& ex) {
			String output = "Exception occurred while checking '" + host->GetName() + "': " + DiagnosticInformation(ex);
			ServiceState state = ServiceUnknown;
			double now = Utility::GetTime();

			if (params->Contains("source")) {
				SendEventExecutedCommand(params, state, output, now, now, listener, origin, sourceEndpoint);
			} else {
				CheckResult::Ptr cr = new CheckResult();
				cr->SetState(state);
				cr->SetOutput(output);
				cr->SetScheduleStart(now);
				cr->SetScheduleEnd(now);
				cr->SetExecutionStart(now);
				cr->SetExecutionEnd(now);

				Dictionary::Ptr message = MakeCheckResultMessage(host, cr);
				listener->SyncSendMessage(sourceEndpoint, message);
			}

			Log(LogCritical, "checker", output);
		}
	} else if (command_type == "event_command") {
		try {
			host->ExecuteEventHandler(macros, true);
		} catch (const std::exception& ex) {
			if (params->Contains("source")) {
				String output = "Exception occurred while executing event command '" + command + "' for '" +
					host->GetName() + "': " + DiagnosticInformation(ex);

				double now = Utility::GetTime();
				SendEventExecutedCommand(params, ServiceUnknown, output, now, now, listener, origin, sourceEndpoint);
			} else {
				throw;
			}
		}
	} else if (command_type == "notification_command" && params->Contains("source")) {
		/* Get user */
		User::Ptr user = new User();
		Dictionary::Ptr attrs = new Dictionary();
		attrs->Set("__name", params->Get("user"));
		attrs->Set("type", User::GetTypeName());

		Deserialize(user, attrs, false, FAConfig);

		/* Get notification */
		Notification::Ptr notification = new Notification();
		attrs->Clear();
		attrs->Set("__name", params->Get("notification"));
		attrs->Set("type", Notification::GetTypeName());
		attrs->Set("command", command);

		Deserialize(notification, attrs, false, FAConfig);

		try {
			CheckResult::Ptr cr = new CheckResult();
			String author = macros->Get("notification_author");
			NotificationCommand::Ptr notificationCommand = NotificationCommand::GetByName(command);

			notificationCommand->Execute(notification, user, cr, NotificationType::NotificationCustom,
				author, "");
		} catch (const std::exception& ex) {
			String output = "Exception occurred during notification '" + notification->GetName()
				+ "' for checkable '" + notification->GetCheckable()->GetName()
				+ "' and user '" + user->GetName() + "' using command '" + command + "': "
				+ DiagnosticInformation(ex, false);
			double now = Utility::GetTime();
			SendEventExecutedCommand(params, ServiceUnknown, output, now, now, listener, origin, sourceEndpoint);
		}
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
