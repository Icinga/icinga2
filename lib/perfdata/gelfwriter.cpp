// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "perfdata/gelfwriter.hpp"
#include "perfdata/gelfwriter-ti.cpp"
#include "icinga/service.hpp"
#include "icinga/notification.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/compatutility.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/application.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
#include "base/context.hpp"
#include "base/exception.hpp"
#include "base/json.hpp"
#include "base/statsfunction.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <utility>
#include "base/io-engine.hpp"
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/error.hpp>

using namespace icinga;

REGISTER_TYPE(GelfWriter);

REGISTER_STATSFUNCTION(GelfWriter, &GelfWriter::StatsFunc);

void GelfWriter::OnConfigLoaded()
{
	ObjectImpl<GelfWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("GelfWriter, " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, "GelfWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void GelfWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const GelfWriter::Ptr& gelfwriter : ConfigType::GetObjectsByType<GelfWriter>()) {
		size_t workQueueItems = gelfwriter->m_WorkQueue.GetLength();
		double workQueueItemRate = gelfwriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		nodes.emplace_back(gelfwriter->GetName(), new Dictionary({
			{ "work_queue_items", workQueueItems },
			{ "work_queue_item_rate", workQueueItemRate },
			{ "connected", gelfwriter->GetConnected() },
			{ "source", gelfwriter->GetSource() }
		}));

		perfdata->Add(new PerfdataValue("gelfwriter_" + gelfwriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("gelfwriter_" + gelfwriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
	}

	status->Set("gelfwriter", new Dictionary(std::move(nodes)));
}

void GelfWriter::Resume()
{
	ObjectImpl<GelfWriter>::Resume();

	Log(LogInformation, "GelfWriter")
		<< "'" << GetName() << "' resumed.";

	/* Register exception handler for WQ tasks. */
	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	/* Timer for reconnecting */
	m_ReconnectTimer = Timer::Create();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect([this](const Timer * const&) { ReconnectTimerHandler(); });
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	/* Register event handlers. */
	m_HandleCheckResults = Checkable::OnNewCheckResult.connect([this](const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
		CheckResultHandler(checkable, cr);
	});
	m_HandleNotifications = Checkable::OnNotificationSentToUser.connect([this](const Notification::Ptr&,
		const Checkable::Ptr& checkable, const User::Ptr&, const NotificationType& type, const CheckResult::Ptr& cr,
		const String& author, const String& commentText, const String& commandName, const MessageOrigin::Ptr&) {
		NotificationToUserHandler(checkable, type, cr, author, commentText, commandName);
	});
	m_HandleStateChanges = Checkable::OnStateChange.connect([this](const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, StateType, const MessageOrigin::Ptr&) {
		StateChangeHandler(checkable, cr);
	});
}

/* Pause is equivalent to Stop, but with HA capabilities to resume at runtime. */
void GelfWriter::Pause()
{
	m_HandleCheckResults.disconnect();
	m_HandleNotifications.disconnect();
	m_HandleStateChanges.disconnect();

	m_ReconnectTimer->Stop(true);

	m_WorkQueue.Enqueue([this]() {
		try {
			ReconnectInternal();
		} catch (const std::exception&) {
			Log(LogInformation, "GelfWriter")
				<< "Unable to connect, not flushing buffers. Data may be lost.";
		}
	}, PriorityImmediate);

	m_WorkQueue.Enqueue([this]() { DisconnectInternal(); }, PriorityLow);
	m_WorkQueue.Join();

	Log(LogInformation, "GelfWriter")
		<< "'" << GetName() << "' paused.";

	ObjectImpl<GelfWriter>::Pause();
}

void GelfWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void GelfWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "GelfWriter") << "Exception during Graylog Gelf operation: " << DiagnosticInformation(exp, false);
	Log(LogDebug, "GelfWriter") << "Exception during Graylog Gelf operation: " << DiagnosticInformation(exp, true);

	DisconnectInternal();
}

void GelfWriter::Reconnect()
{
	AssertOnWorkQueue();

	if (IsPaused()) {
		SetConnected(false);
		return;
	}

	ReconnectInternal();
}

void GelfWriter::ReconnectInternal()
{
	double startTime = Utility::GetTime();

	CONTEXT("Reconnecting to Graylog Gelf '" << GetName() << "'");

	SetShouldConnect(true);

	if (GetConnected())
		return;

	Log(LogNotice, "GelfWriter")
		<< "Reconnecting to Graylog Gelf on host '" << GetHost() << "' port '" << GetPort() << "'.";

	bool ssl = GetEnableTls();

	if (ssl) {
		Shared<boost::asio::ssl::context>::Ptr sslContext;

		try {
			sslContext = MakeAsioSslContext(GetCertPath(), GetKeyPath(), GetCaPath());
		} catch (const std::exception& ex) {
			Log(LogWarning, "GelfWriter")
				<< "Unable to create SSL context.";
			throw;
		}

		m_Stream.first = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslContext, GetHost());

	} else {
		m_Stream.second = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());
	}

	try {
		icinga::Connect(ssl ? m_Stream.first->lowest_layer() : m_Stream.second->lowest_layer(), GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogWarning, "GelfWriter")
			<< "Can't connect to Graylog Gelf on host '" << GetHost() << "' port '" << GetPort() << ".'";
		throw;
	}

	if (ssl) {
		auto& tlsStream (m_Stream.first->next_layer());

		try {
			tlsStream.handshake(tlsStream.client);
		} catch (const std::exception& ex) {
			Log(LogWarning, "GelfWriter")
				<< "TLS handshake with host '" << GetHost() << " failed.'";
			throw;
		}

		if (!GetInsecureNoverify()) {
			if (!tlsStream.GetPeerCertificate()) {
				BOOST_THROW_EXCEPTION(std::runtime_error("Graylog Gelf didn't present any TLS certificate."));
			}

			if (!tlsStream.IsVerifyOK()) {
				BOOST_THROW_EXCEPTION(std::runtime_error(
					"TLS certificate validation failed: " + std::string(tlsStream.GetVerifyError())
				));
			}
		}
	}

	SetConnected(true);

	Log(LogInformation, "GelfWriter")
		<< "Finished reconnecting to Graylog Gelf in " << std::setw(2) << Utility::GetTime() - startTime << " second(s).";
}

void GelfWriter::ReconnectTimerHandler()
{
	m_WorkQueue.Enqueue([this]() { Reconnect(); }, PriorityNormal);
}

void GelfWriter::Disconnect()
{
	AssertOnWorkQueue();

	DisconnectInternal();
}

void GelfWriter::DisconnectInternal()
{
	if (!GetConnected())
		return;

	if (m_Stream.first) {
		boost::system::error_code ec;
		m_Stream.first->next_layer().shutdown(ec);

		// https://stackoverflow.com/a/25703699
		// As long as the error code's category is not an SSL category, then the protocol was securely shutdown
		if (ec.category() == boost::asio::error::get_ssl_category()) {
			Log(LogCritical, "GelfWriter")
				<< "TLS shutdown with host '" << GetHost() << "' could not be done securely.";
		}
	} else if (m_Stream.second) {
		m_Stream.second->close();
	}

	SetConnected(false);

}

void GelfWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("_service_name", service->GetShortName());
		fields->Set("_service_state", Service::StateToString(service->GetState()));
		fields->Set("_last_state", service->GetLastState());
		fields->Set("_last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("_last_state", host->GetLastState());
		fields->Set("_last_hard_state", host->GetLastHardState());
	}

	fields->Set("_hostname", host->GetName());
	fields->Set("_type", "CHECK RESULT");
	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("_current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("_max_check_attempts", checkable->GetMaxCheckAttempts());

	fields->Set("_reachable", checkable->IsReachable());

	CheckCommand::Ptr checkCommand = checkable->GetCheckCommand();
	fields->Set("_check_command", checkCommand->GetName());

	m_WorkQueue.Enqueue([this, checkable, cr, fields = std::move(fields)]() {
		CONTEXT("GELF Processing check result for '" << checkable->GetName() << "'");

		Log(LogDebug, "GelfWriter")
			<< "Processing check result for '" << checkable->GetName() << "'";

		fields->Set("_latency", cr->CalculateLatency());
		fields->Set("_execution_time", cr->CalculateExecutionTime());
		fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
		fields->Set("full_message", cr->GetOutput());
		fields->Set("_check_source", cr->GetCheckSource());

		if (GetEnableSendPerfdata()) {
			Array::Ptr perfdata = cr->GetPerformanceData();

			if (perfdata) {
				ObjectLock olock(perfdata);
				for (const Value& val : perfdata) {
					PerfdataValue::Ptr pdv;

					if (val.IsObjectType<PerfdataValue>())
						pdv = val;
					else {
						try {
							pdv = PerfdataValue::Parse(val);
						} catch (const std::exception&) {
							Log(LogWarning, "GelfWriter")
								<< "Ignoring invalid perfdata for checkable '"
								<< checkable->GetName() << "' and command '"
								<< checkable->GetCheckCommand()->GetName() << "' with value: " << val;
							continue;
						}
					}

					String escaped_key = pdv->GetLabel();
					boost::replace_all(escaped_key, " ", "_");
					boost::replace_all(escaped_key, ".", "_");
					boost::replace_all(escaped_key, "\\", "_");
					boost::algorithm::replace_all(escaped_key, "::", ".");

					fields->Set("_" + escaped_key, pdv->GetValue());

					if (!pdv->GetMin().IsEmpty())
						fields->Set("_" + escaped_key + "_min", pdv->GetMin());
					if (!pdv->GetMax().IsEmpty())
						fields->Set("_" + escaped_key + "_max", pdv->GetMax());
					if (!pdv->GetWarn().IsEmpty())
						fields->Set("_" + escaped_key + "_warn", pdv->GetWarn());
					if (!pdv->GetCrit().IsEmpty())
						fields->Set("_" + escaped_key + "_crit", pdv->GetCrit());

					if (!pdv->GetUnit().IsEmpty())
						fields->Set("_" + escaped_key + "_unit", pdv->GetUnit());
				}
			}
		}

		SendLogMessage(checkable, ComposeGelfMessage(fields, GetSource(), cr->GetExecutionEnd()));
	});
}

void GelfWriter::NotificationToUserHandler(const Checkable::Ptr& checkable, NotificationType notificationType,
	const CheckResult::Ptr& cr, const String& author, const String& commentText, const String& commandName)
{
	if (IsPaused())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String notificationTypeString = Notification::NotificationTypeToStringCompat(notificationType); //TODO: Change that to our own types.

	String authorComment = "";

	if (notificationType == NotificationCustom || notificationType == NotificationAcknowledgement) {
		authorComment = author + ";" + commentText;
	}

	String output;
	double ts = Utility::GetTime();

	if (cr) {
		output = CompatUtility::GetCheckResultOutput(cr);
		ts = cr->GetExecutionEnd();
	}

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("_type", "SERVICE NOTIFICATION");
		//TODO: fix this to _service_name
		fields->Set("_service", service->GetShortName());
		fields->Set("short_message", output);
	} else {
		fields->Set("_type", "HOST NOTIFICATION");
		fields->Set("short_message", output);
	}

	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));

	fields->Set("_hostname", host->GetName());
	fields->Set("_command", commandName);
	fields->Set("_notification_type", notificationTypeString);
	fields->Set("_comment", authorComment);
	fields->Set("_check_command", checkable->GetCheckCommand()->GetName());

	m_WorkQueue.Enqueue([this, checkable, ts, fields = std::move(fields)]() {
		CONTEXT("GELF Processing notification to all users '" << checkable->GetName() << "'");

		Log(LogDebug, "GelfWriter")
			<< "Processing notification for '" << checkable->GetName() << "'";

		SendLogMessage(checkable, ComposeGelfMessage(fields, GetSource(), ts));
	});
}

void GelfWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	fields->Set("_state", service ? Service::StateToString(service->GetState()) : Host::StateToString(host->GetState()));
	fields->Set("_type", "STATE CHANGE");
	fields->Set("_current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("_max_check_attempts", checkable->GetMaxCheckAttempts());
	fields->Set("_hostname", host->GetName());

	if (service) {
		fields->Set("_service_name", service->GetShortName());
		fields->Set("_service_state", Service::StateToString(service->GetState()));
		fields->Set("_last_state", service->GetLastState());
		fields->Set("_last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("_last_state", host->GetLastState());
		fields->Set("_last_hard_state", host->GetLastHardState());
	}

	fields->Set("_check_command", checkable->GetCheckCommand()->GetName());
	fields->Set("short_message", CompatUtility::GetCheckResultOutput(cr));
	fields->Set("full_message", cr->GetOutput());
	fields->Set("_check_source", cr->GetCheckSource());

	m_WorkQueue.Enqueue([this, checkable, fields = std::move(fields), ts = cr->GetExecutionEnd()]() {
		CONTEXT("GELF Processing state change '" << checkable->GetName() << "'");

		Log(LogDebug, "GelfWriter")
			<< "Processing state change for '" << checkable->GetName() << "'";

		SendLogMessage(checkable, ComposeGelfMessage(fields, GetSource(), ts));
	});
}

String GelfWriter::ComposeGelfMessage(const Dictionary::Ptr& fields, const String& source, double ts)
{
	fields->Set("version", "1.1");
	fields->Set("host", source);
	fields->Set("timestamp", ts);

	return JsonEncode(fields);
}

void GelfWriter::SendLogMessage(const Checkable::Ptr& checkable, const String& gelfMessage)
{
	AssertOnWorkQueue();

	std::ostringstream msgbuf;
	msgbuf << gelfMessage;
	msgbuf << '\0';

	String log = msgbuf.str();

	if (!GetConnected())
		return;

	try {
		Log(LogDebug, "GelfWriter")
			<< "Checkable '" << checkable->GetName() << "' sending message '" << log << "'.";

		if (m_Stream.first) {
			boost::asio::write(*m_Stream.first, boost::asio::buffer(msgbuf.str()));
			m_Stream.first->flush();
		} else {
			boost::asio::write(*m_Stream.second, boost::asio::buffer(msgbuf.str()));
			m_Stream.second->flush();
		}
	} catch (const std::exception& ex) {
		Log(LogCritical, "GelfWriter")
			<< "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

		throw ex;
	}
}
