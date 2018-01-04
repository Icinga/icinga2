/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "perfdata/elasticsearchwriter.hpp"
#include "perfdata/elasticsearchwriter.tcpp"
#include "remote/url.hpp"
#include "remote/httprequest.hpp"
#include "remote/httpresponse.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/service.hpp"
#include "icinga/checkcommand.hpp"
#include "base/tcpsocket.hpp"
#include "base/stream.hpp"
#include "base/base64.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"
#include "base/networkstream.hpp"
#include "base/perfdatavalue.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/scoped_array.hpp>
#include <utility>

using namespace icinga;

REGISTER_TYPE(ElasticsearchWriter);

REGISTER_STATSFUNCTION(ElasticsearchWriter, &ElasticsearchWriter::StatsFunc);

ElasticsearchWriter::ElasticsearchWriter()
	: m_WorkQueue(10000000, 1)
{ }

void ElasticsearchWriter::OnConfigLoaded()
{
	ObjectImpl<ElasticsearchWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("ElasticsearchWriter, " + GetName());
}

void ElasticsearchWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const ElasticsearchWriter::Ptr& elasticsearchwriter : ConfigType::GetObjectsByType<ElasticsearchWriter>()) {
		size_t workQueueItems = elasticsearchwriter->m_WorkQueue.GetLength();
		double workQueueItemRate = elasticsearchwriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("work_queue_items", workQueueItems);
		stats->Set("work_queue_item_rate", workQueueItemRate);

		nodes->Set(elasticsearchwriter->GetName(), stats);

		perfdata->Add(new PerfdataValue("elasticsearchwriter_" + elasticsearchwriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("elasticsearchwriter_" + elasticsearchwriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
	}

	status->Set("elasticsearchwriter", nodes);
}

void ElasticsearchWriter::Start(bool runtimeCreated)
{
	ObjectImpl<ElasticsearchWriter>::Start(runtimeCreated);

	m_EventPrefix = "icinga2.event.";

	Log(LogInformation, "ElasticsearchWriter")
		<< "'" << GetName() << "' started.";

	m_WorkQueue.SetExceptionCallback(std::bind(&ElasticsearchWriter::ExceptionHandler, this, _1));

	/* Setup timer for periodically flushing m_DataBuffer */
	m_FlushTimer = new Timer();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect(std::bind(&ElasticsearchWriter::FlushTimeout, this));
	m_FlushTimer->Start();
	m_FlushTimer->Reschedule(0);

	/* Register for new metrics. */
	Checkable::OnNewCheckResult.connect(std::bind(&ElasticsearchWriter::CheckResultHandler, this, _1, _2));
	Checkable::OnStateChange.connect(std::bind(&ElasticsearchWriter::StateChangeHandler, this, _1, _2, _3));
	Checkable::OnNotificationSentToAllUsers.connect(std::bind(&ElasticsearchWriter::NotificationSentToAllUsersHandler, this, _1, _2, _3, _4, _5, _6, _7));
}

void ElasticsearchWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "ElasticsearchWriter")
		<< "'" << GetName() << "' stopped.";

	m_WorkQueue.Join();

	ObjectImpl<ElasticsearchWriter>::Stop(runtimeRemoved);
}

void ElasticsearchWriter::AddCheckResult(const Dictionary::Ptr& fields, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	String prefix = "check_result.";

	fields->Set(prefix + "output", cr->GetOutput());
	fields->Set(prefix + "check_source", cr->GetCheckSource());
	fields->Set(prefix + "exit_status", cr->GetExitStatus());
	fields->Set(prefix + "command", cr->GetCommand());
	fields->Set(prefix + "state", cr->GetState());
	fields->Set(prefix + "vars_before", cr->GetVarsBefore());
	fields->Set(prefix + "vars_after", cr->GetVarsAfter());

	fields->Set(prefix + "execution_start", FormatTimestamp(cr->GetExecutionStart()));
	fields->Set(prefix + "execution_end", FormatTimestamp(cr->GetExecutionEnd()));
	fields->Set(prefix + "schedule_start", FormatTimestamp(cr->GetScheduleStart()));
	fields->Set(prefix + "schedule_end", FormatTimestamp(cr->GetScheduleEnd()));

	/* Add extra calculated field. */
	fields->Set(prefix + "latency", cr->CalculateLatency());
	fields->Set(prefix + "execution_time", cr->CalculateExecutionTime());

	if (!GetEnableSendPerfdata())
		return;

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
					Log(LogWarning, "ElasticsearchWriter")
						<< "Ignoring invalid perfdata value: '" << val << "' for object '"
						<< checkable->GetName() << "'.";
				}
			}

			String escapedKey = pdv->GetLabel();
			boost::replace_all(escapedKey, " ", "_");
			boost::replace_all(escapedKey, ".", "_");
			boost::replace_all(escapedKey, "\\", "_");
			boost::algorithm::replace_all(escapedKey, "::", ".");

			String perfdataPrefix = prefix + "perfdata." + escapedKey;

			fields->Set(perfdataPrefix + ".value", pdv->GetValue());

			if (pdv->GetMin())
				fields->Set(perfdataPrefix + ".min", pdv->GetMin());
			if (pdv->GetMax())
				fields->Set(perfdataPrefix + ".max", pdv->GetMax());
			if (pdv->GetWarn())
				fields->Set(perfdataPrefix + ".warn", pdv->GetWarn());
			if (pdv->GetCrit())
				fields->Set(perfdataPrefix + ".crit", pdv->GetCrit());
		}
	}
}

void ElasticsearchWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	m_WorkQueue.Enqueue(std::bind(&ElasticsearchWriter::InternalCheckResultHandler, this, checkable, cr));
}

void ElasticsearchWriter::InternalCheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	CONTEXT("Elasticwriter processing check result for '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("service", service->GetShortName());
		fields->Set("state", service->GetState());
		fields->Set("last_state", service->GetLastState());
		fields->Set("last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("state", host->GetState());
		fields->Set("last_state", host->GetLastState());
		fields->Set("last_hard_state", host->GetLastHardState());
	}

	fields->Set("host", host->GetName());
	fields->Set("state_type", checkable->GetStateType());

	fields->Set("current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("max_check_attempts", checkable->GetMaxCheckAttempts());

	fields->Set("reachable", checkable->IsReachable());

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	if (commandObj)
		fields->Set("check_command", commandObj->GetName());

	double ts = Utility::GetTime();

	if (cr) {
		AddCheckResult(fields, checkable, cr);
		ts = cr->GetExecutionEnd();
	}

	Enqueue("checkresult", fields, ts);
}

void ElasticsearchWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	m_WorkQueue.Enqueue(std::bind(&ElasticsearchWriter::StateChangeHandlerInternal, this, checkable, cr, type));
}

void ElasticsearchWriter::StateChangeHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	AssertOnWorkQueue();

	CONTEXT("Elasticwriter processing state change '" + checkable->GetName() + "'");

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr fields = new Dictionary();

	fields->Set("current_check_attempt", checkable->GetCheckAttempt());
	fields->Set("max_check_attempts", checkable->GetMaxCheckAttempts());
	fields->Set("host", host->GetName());

	if (service) {
		fields->Set("service", service->GetShortName());
		fields->Set("state", service->GetState());
		fields->Set("last_state", service->GetLastState());
		fields->Set("last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("state", host->GetState());
		fields->Set("last_state", host->GetLastState());
		fields->Set("last_hard_state", host->GetLastHardState());
	}

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	if (commandObj)
		fields->Set("check_command", commandObj->GetName());

	double ts = Utility::GetTime();

	if (cr) {
		AddCheckResult(fields, checkable, cr);
		ts = cr->GetExecutionEnd();
	}

	Enqueue("statechange", fields, ts);
}

void ElasticsearchWriter::NotificationSentToAllUsersHandler(const Notification::Ptr& notification,
	const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text)
{
	m_WorkQueue.Enqueue(std::bind(&ElasticsearchWriter::NotificationSentToAllUsersHandlerInternal, this,
		notification, checkable, users, type, cr, author, text));
}

void ElasticsearchWriter::NotificationSentToAllUsersHandlerInternal(const Notification::Ptr& notification,
	const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text)
{
	AssertOnWorkQueue();

	CONTEXT("Elasticwriter processing notification to all users '" + checkable->GetName() + "'");

	Log(LogDebug, "ElasticsearchWriter")
		<< "Processing notification for '" << checkable->GetName() << "'";

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	String notificationTypeString = Notification::NotificationTypeToString(type);

	Dictionary::Ptr fields = new Dictionary();

	if (service) {
		fields->Set("service", service->GetShortName());
		fields->Set("state", service->GetState());
		fields->Set("last_state", service->GetLastState());
		fields->Set("last_hard_state", service->GetLastHardState());
	} else {
		fields->Set("state", host->GetState());
		fields->Set("last_state", host->GetLastState());
		fields->Set("last_hard_state", host->GetLastHardState());
	}

	fields->Set("host", host->GetName());

	Array::Ptr userNames = new Array();

	for (const User::Ptr& user : users) {
		userNames->Add(user->GetName());
	}

	fields->Set("users", userNames);
	fields->Set("notification_type", notificationTypeString);
	fields->Set("author", author);
	fields->Set("text", text);

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	if (commandObj)
		fields->Set("check_command", commandObj->GetName());

	double ts = Utility::GetTime();

	if (cr) {
		AddCheckResult(fields, checkable, cr);
		ts = cr->GetExecutionEnd();
	}

	Enqueue("notification", fields, ts);
}

void ElasticsearchWriter::Enqueue(const String& type, const Dictionary::Ptr& fields, double ts)
{
	/* Atomically buffer the data point. */
	boost::mutex::scoped_lock lock(m_DataBufferMutex);

	/* Format the timestamps to dynamically select the date datatype inside the index. */
	fields->Set("@timestamp", FormatTimestamp(ts));
	fields->Set("timestamp", FormatTimestamp(ts));

	String eventType = m_EventPrefix + type;
	fields->Set("type", eventType);

	/* Every payload needs a line describing the index above.
	 * We do it this way to avoid problems with a near full queue.
	 */

	String indexBody = R"({ "index" : { "_type" : ")" + eventType + "\" } }\n";
	String fieldsBody = JsonEncode(fields);

	Log(LogDebug, "ElasticsearchWriter")
		<< "Add to fields to message list: '" << fieldsBody << "'.";

	m_DataBuffer.emplace_back(indexBody + fieldsBody);

	/* Flush if we've buffered too much to prevent excessive memory use. */
	if (static_cast<int>(m_DataBuffer.size()) >= GetFlushThreshold()) {
		Log(LogDebug, "ElasticsearchWriter")
			<< "Data buffer overflow writing " << m_DataBuffer.size() << " data points";
		Flush();
	}
}

void ElasticsearchWriter::FlushTimeout()
{
	/* Prevent new data points from being added to the array, there is a
	 * race condition where they could disappear.
	 */
	boost::mutex::scoped_lock lock(m_DataBufferMutex);

	/* Flush if there are any data available. */
	if (m_DataBuffer.size() > 0) {
		Log(LogDebug, "ElasticsearchWriter")
			<< "Timer expired writing " << m_DataBuffer.size() << " data points";
		Flush();
	}
}

void ElasticsearchWriter::Flush()
{
	/* Ensure you hold a lock against m_DataBuffer so that things
	 * don't go missing after creating the body and clearing the buffer.
	 */
	String body = boost::algorithm::join(m_DataBuffer, "\n");
	m_DataBuffer.clear();

	/* Elasticsearch 6.x requires a new line. This is compatible to 5.x.
	 * Tested with 6.0.0 and 5.6.4.
	 */
	body += "\n";

	SendRequest(body);
}

void ElasticsearchWriter::SendRequest(const String& body)
{
	Url::Ptr url = new Url();

	url->SetScheme(GetEnableTls() ? "https" : "http");
	url->SetHost(GetHost());
	url->SetPort(GetPort());

	std::vector<String> path;

	/* Specify the index path. Best practice is a daily rotation.
	 * Example: http://localhost:9200/icinga2-2017.09.11?pretty=1
	 */
	path.emplace_back(GetIndex() + "-" + Utility::FormatDateTime("%Y.%m.%d", Utility::GetTime()));

	/* Use the bulk message format. */
	path.emplace_back("_bulk");

	url->SetPath(path);

	Stream::Ptr stream = Connect();
	HttpRequest req(stream);

	/* Specify required headers by Elasticsearch. */
	req.AddHeader("Accept", "application/json");
	req.AddHeader("Content-Type", "application/json");

	/* Send authentication if configured. */
	String username = GetUsername();
	String password = GetPassword();

	if (!username.IsEmpty() && !password.IsEmpty())
		req.AddHeader("Authorization", "Basic " + Base64::Encode(username + ":" + password));

	req.RequestMethod = "POST";
	req.RequestUrl = url;

	/* Don't log the request body to debug log, this is already done above. */
	Log(LogDebug, "ElasticsearchWriter")
		<< "Sending " << req.RequestMethod << " request" << ((!username.IsEmpty() && !password.IsEmpty()) ? " with basic auth" : "" )
		<< " to '" << url->Format() << "'.";

	try {
		req.WriteBody(body.CStr(), body.GetLength());
		req.Finish();
	} catch (const std::exception& ex) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Cannot write to HTTP API on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	HttpResponse resp(stream, req);
	StreamReadContext context;

	try {
		resp.Parse(context, true);
		while (resp.Parse(context, true) && !resp.Complete)
			; /* Do nothing */
	} catch (const std::exception& ex) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Failed to parse HTTP response from host '" << GetHost() << "' port '" << GetPort() << "': " << DiagnosticInformation(ex, false);
		throw ex;
	}

	if (!resp.Complete) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Failed to read a complete HTTP response from the Elasticsearch server.";
		return;
	}

	if (resp.StatusCode > 299) {
		if (resp.StatusCode == 401) {
			/* More verbose error logging with Elasticsearch is hidden behind a proxy. */
			if (!username.IsEmpty() && !password.IsEmpty()) {
				Log(LogCritical, "ElasticsearchWriter")
					<< "401 Unauthorized. Please ensure that the user '" << username
					<< "' is able to authenticate against the HTTP API/Proxy.";
			} else {
				Log(LogCritical, "ElasticsearchWriter")
					<< "401 Unauthorized. The HTTP API requires authentication but no username/password has been configured.";
			}

			return;
		}

		Log(LogWarning, "ElasticsearchWriter")
			<< "Unexpected response code " << resp.StatusCode;

		String contentType = resp.Headers->Get("content-type");

		if (contentType != "application/json") {
			Log(LogWarning, "ElasticsearchWriter")
				<< "Unexpected Content-Type: " << contentType;
			return;
		}

		size_t responseSize = resp.GetBodySize();
		boost::scoped_array<char> buffer(new char[responseSize + 1]);
		resp.ReadBody(buffer.get(), responseSize);
		buffer.get()[responseSize] = '\0';

		Dictionary::Ptr jsonResponse;
		try {
			jsonResponse = JsonDecode(buffer.get());
		} catch (...) {
			Log(LogWarning, "ElasticsearchWriter")
				<< "Unable to parse JSON response:\n" << buffer.get();
			return;
		}

		String error = jsonResponse->Get("error");

		Log(LogCritical, "ElasticsearchWriter")
			<< "Elasticsearch error message:\n" << error;

		return;
	}
}

Stream::Ptr ElasticsearchWriter::Connect()
{
	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "ElasticsearchWriter")
		<< "Connecting to Elasticsearch on host '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Can't connect to Elasticsearch on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	if (GetEnableTls()) {
		std::shared_ptr<SSL_CTX> sslContext;

		try {
			sslContext = MakeSSLContext(GetCertPath(), GetKeyPath(), GetCaPath());
		} catch (const std::exception& ex) {
			Log(LogWarning, "ElasticsearchWriter")
				<< "Unable to create SSL context.";
			throw ex;
		}

		TlsStream::Ptr tlsStream = new TlsStream(socket, GetHost(), RoleClient, sslContext);

		try {
			tlsStream->Handshake();
		} catch (const std::exception& ex) {
			Log(LogWarning, "ElasticsearchWriter")
				<< "TLS handshake with host '" << GetHost() << "' on port " << GetPort() << " failed.";
			throw ex;
		}

		return tlsStream;
	} else {
		return new NetworkStream(socket);
	}
}

void ElasticsearchWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void ElasticsearchWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "ElasticsearchWriter", "Exception during Elastic operation: Verify that your backend is operational!");

	Log(LogDebug, "ElasticsearchWriter")
		<< "Exception during Elasticsearch operation: " << DiagnosticInformation(std::move(exp));
}

String ElasticsearchWriter::FormatTimestamp(double ts)
{
	/* The date format must match the default dynamic date detection
	 * pattern in indexes. This enables applications like Kibana to
	 * detect a qualified timestamp index for time-series data.
	 *
	 * Example: 2017-09-11T10:56:21.463+0200
	 *
	 * References:
	 * https://www.elastic.co/guide/en/elasticsearch/reference/current/dynamic-field-mapping.html#date-detection
	 * https://www.elastic.co/guide/en/elasticsearch/reference/current/mapping-date-format.html
	 * https://www.elastic.co/guide/en/elasticsearch/reference/current/date.html
	 */
	int milliSeconds = static_cast<int>((ts - static_cast<int>(ts)) * 1000);

	return Utility::FormatDateTime("%Y-%m-%dT%H:%M:%S", ts) + "." + Convert::ToString(milliSeconds) + Utility::FormatDateTime("%z", ts);
}
