/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "perfdata/elasticsearchwriter.hpp"
#include "perfdata/elasticsearchwriter-ti.cpp"
#include "remote/url.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/service.hpp"
#include "icinga/checkcommand.hpp"
#include "base/application.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
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
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/scoped_array.hpp>
#include <memory>
#include <string>
#include <utility>

using namespace icinga;

REGISTER_TYPE(ElasticsearchWriter);

REGISTER_STATSFUNCTION(ElasticsearchWriter, &ElasticsearchWriter::StatsFunc);

void ElasticsearchWriter::OnConfigLoaded()
{
	ObjectImpl<ElasticsearchWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("ElasticsearchWriter, " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, "ElasticsearchWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void ElasticsearchWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const ElasticsearchWriter::Ptr& elasticsearchwriter : ConfigType::GetObjectsByType<ElasticsearchWriter>()) {
		size_t workQueueItems = elasticsearchwriter->m_WorkQueue.GetLength();
		double workQueueItemRate = elasticsearchwriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		nodes.emplace_back(elasticsearchwriter->GetName(), new Dictionary({
			{ "work_queue_items", workQueueItems },
			{ "work_queue_item_rate", workQueueItemRate }
		}));

		perfdata->Add(new PerfdataValue("elasticsearchwriter_" + elasticsearchwriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("elasticsearchwriter_" + elasticsearchwriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
	}

	status->Set("elasticsearchwriter", new Dictionary(std::move(nodes)));
}

void ElasticsearchWriter::Resume()
{
	ObjectImpl<ElasticsearchWriter>::Resume();

	m_EventPrefix = "icinga2.event.";

	Log(LogInformation, "ElasticsearchWriter")
		<< "'" << GetName() << "' resumed.";

	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	/* Setup timer for periodically flushing m_DataBuffer */
	m_FlushTimer = new Timer();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect([this](const Timer * const&) { FlushTimeout(); });
	m_FlushTimer->Start();
	m_FlushTimer->Reschedule(0);

	/* Register for new metrics. */
	Checkable::OnNewCheckResult.connect([this](const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
		CheckResultHandler(checkable, cr);
	});
	Checkable::OnStateChange.connect([this](const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type,
		const MessageOrigin::Ptr&) {
		StateChangeHandler(checkable, cr, type);
	});
	Checkable::OnNotificationSentToAllUsers.connect([this](const Notification::Ptr& notification, const Checkable::Ptr& checkable,
		const std::set<User::Ptr>& users, const NotificationType& type, const CheckResult::Ptr& cr, const String& author,
		const String& text, const MessageOrigin::Ptr&) {
		NotificationSentToAllUsersHandler(notification, checkable, users, type, cr, author, text);
	});
}

/* Pause is equivalent to Stop, but with HA capabilities to resume at runtime. */
void ElasticsearchWriter::Pause()
{
	Flush();
	m_WorkQueue.Join();
	Flush();

	Log(LogInformation, "ElasticsearchWriter")
		<< "'" << GetName() << "' paused.";

	ObjectImpl<ElasticsearchWriter>::Pause();
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

	CheckCommand::Ptr checkCommand = checkable->GetCheckCommand();

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
						<< "Ignoring invalid perfdata for checkable '"
						<< checkable->GetName() << "' and command '"
						<< checkCommand->GetName() << "' with value: " << val;
					continue;
				}
			}

			String escapedKey = pdv->GetLabel();
			boost::replace_all(escapedKey, " ", "_");
			boost::replace_all(escapedKey, ".", "_");
			boost::replace_all(escapedKey, "\\", "_");
			boost::algorithm::replace_all(escapedKey, "::", ".");

			String perfdataPrefix = prefix + "perfdata." + escapedKey;

			fields->Set(perfdataPrefix + ".value", pdv->GetValue());

			if (!pdv->GetMin().IsEmpty())
				fields->Set(perfdataPrefix + ".min", pdv->GetMin());
			if (!pdv->GetMax().IsEmpty())
				fields->Set(perfdataPrefix + ".max", pdv->GetMax());
			if (!pdv->GetWarn().IsEmpty())
				fields->Set(perfdataPrefix + ".warn", pdv->GetWarn());
			if (!pdv->GetCrit().IsEmpty())
				fields->Set(perfdataPrefix + ".crit", pdv->GetCrit());

			if (!pdv->GetUnit().IsEmpty())
				fields->Set(perfdataPrefix + ".unit", pdv->GetUnit());
		}
	}
}

void ElasticsearchWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	m_WorkQueue.Enqueue([this, checkable, cr]() { InternalCheckResultHandler(checkable, cr); });
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

	Enqueue(checkable, "checkresult", fields, ts);
}

void ElasticsearchWriter::StateChangeHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, StateType type)
{
	if (IsPaused())
		return;

	m_WorkQueue.Enqueue([this, checkable, cr, type]() { StateChangeHandlerInternal(checkable, cr, type); });
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

	Enqueue(checkable, "statechange", fields, ts);
}

void ElasticsearchWriter::NotificationSentToAllUsersHandler(const Notification::Ptr& notification,
	const Checkable::Ptr& checkable, const std::set<User::Ptr>& users, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text)
{
	if (IsPaused())
		return;

	m_WorkQueue.Enqueue([this, notification, checkable, users, type, cr, author, text]() {
		NotificationSentToAllUsersHandlerInternal(notification, checkable, users, type, cr, author, text);
	});
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

	String notificationTypeString = Notification::NotificationTypeToStringCompat(type); //TODO: Change that to our own types.

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

	ArrayData userNames;

	for (const User::Ptr& user : users) {
		userNames.push_back(user->GetName());
	}

	fields->Set("users", new Array(std::move(userNames)));
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

	Enqueue(checkable, "notification", fields, ts);
}

void ElasticsearchWriter::Enqueue(const Checkable::Ptr& checkable, const String& type,
	const Dictionary::Ptr& fields, double ts)
{
	/* Atomically buffer the data point. */
	boost::mutex::scoped_lock lock(m_DataBufferMutex);

	/* Format the timestamps to dynamically select the date datatype inside the index. */
	fields->Set("@timestamp", FormatTimestamp(ts));
	fields->Set("timestamp", FormatTimestamp(ts));

	String eventType = m_EventPrefix + type;
	fields->Set("type", eventType);

	/* Every payload needs a line describing the index.
	 * We do it this way to avoid problems with a near full queue.
	 */
	String indexBody = "{\"index\": {} }\n";
	String fieldsBody = JsonEncode(fields);

	Log(LogDebug, "ElasticsearchWriter")
		<< "Checkable '" << checkable->GetName() << "' adds to metric list: '" << fieldsBody << "'.";

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
	/* Flush can be called from 1) Timeout 2) Threshold 3) on shutdown/reload. */
	if (m_DataBuffer.empty())
		return;

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
	namespace beast = boost::beast;
	namespace http = beast::http;

	Url::Ptr url = new Url();

	url->SetScheme(GetEnableTls() ? "https" : "http");
	url->SetHost(GetHost());
	url->SetPort(GetPort());

	std::vector<String> path;

	/* Specify the index path. Best practice is a daily rotation.
	 * Example: http://localhost:9200/icinga2-2017.09.11?pretty=1
	 */
	path.emplace_back(GetIndex() + "-" + Utility::FormatDateTime("%Y.%m.%d", Utility::GetTime()));

	/* ES 6 removes multiple _type mappings: https://www.elastic.co/guide/en/elasticsearch/reference/6.x/removal-of-types.html
	 * Best practice is to statically define 'doc', as ES 5.X does not allow types starting with '_'.
	 */
	path.emplace_back("doc");

	/* Use the bulk message format. */
	path.emplace_back("_bulk");

	url->SetPath(path);

	OptionalTlsStream stream;

	try {
		stream = Connect();
	} catch (const std::exception& ex) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Flush failed, cannot connect to Elasticsearch: " << DiagnosticInformation(ex, false);
		return;
	}

	Defer s ([&stream]() {
		if (stream.first) {
			stream.first->next_layer().shutdown();
		}
	});

	http::request<http::string_body> request (http::verb::post, std::string(url->Format(true)), 10);

	request.set(http::field::user_agent, "Icinga/" + Application::GetAppVersion());
	request.set(http::field::host, url->GetHost() + ":" + url->GetPort());

	/* Specify required headers by Elasticsearch. */
	request.set(http::field::accept, "application/json");

	/* Use application/x-ndjson for bulk streams. While ES
	 * is able to handle application/json, the newline separator
	 * causes problems with Logstash (#6609).
	 */
	request.set(http::field::content_type, "application/x-ndjson");

	/* Send authentication if configured. */
	String username = GetUsername();
	String password = GetPassword();

	if (!username.IsEmpty() && !password.IsEmpty())
		request.set(http::field::authorization, "Basic " + Base64::Encode(username + ":" + password));

	request.body() = body;
	request.content_length(request.body().size());

	/* Don't log the request body to debug log, this is already done above. */
	Log(LogDebug, "ElasticsearchWriter")
		<< "Sending " << request.method_string() << " request" << ((!username.IsEmpty() && !password.IsEmpty()) ? " with basic auth" : "" )
		<< " to '" << url->Format() << "'.";

	try {
		if (stream.first) {
			http::write(*stream.first, request);
			stream.first->flush();
		} else {
			http::write(*stream.second, request);
			stream.second->flush();
		}
	} catch (const std::exception&) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Cannot write to HTTP API on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw;
	}

	http::parser<false, http::string_body> parser;
	beast::flat_buffer buf;

	try {
		if (stream.first) {
			http::read(*stream.first, buf, parser);
		} else {
			http::read(*stream.second, buf, parser);
		}
	} catch (const std::exception& ex) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Failed to parse HTTP response from host '" << GetHost() << "' port '" << GetPort() << "': " << DiagnosticInformation(ex, false);
		throw;
	}

	auto& response (parser.get());

	if (response.result_int() > 299) {
		if (response.result() == http::status::unauthorized) {
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

		std::ostringstream msgbuf;
		msgbuf << "Unexpected response code " << response.result_int() << " from URL '" << url->Format() << "'";

		auto& contentType (response[http::field::content_type]);

		if (contentType != "application/json" && contentType != "application/json; charset=utf-8") {
			msgbuf << "; Unexpected Content-Type: '" << contentType << "'";
		}

		auto& body (response.body());

#ifdef I2_DEBUG
		msgbuf << "; Response body: '" << body << "'";
#endif /* I2_DEBUG */

		Dictionary::Ptr jsonResponse;

		try {
			jsonResponse = JsonDecode(body);
		} catch (...) {
			Log(LogWarning, "ElasticsearchWriter")
				<< "Unable to parse JSON response:\n" << body;
			return;
		}

		String error = jsonResponse->Get("error");

		Log(LogCritical, "ElasticsearchWriter")
			<< "Error: '" << error << "'. " << msgbuf.str();
	}
}

OptionalTlsStream ElasticsearchWriter::Connect()
{
	Log(LogNotice, "ElasticsearchWriter")
		<< "Connecting to Elasticsearch on host '" << GetHost() << "' port '" << GetPort() << "'.";

	OptionalTlsStream stream;
	bool tls = GetEnableTls();

	if (tls) {
		Shared<boost::asio::ssl::context>::Ptr sslContext;

		try {
			sslContext = MakeAsioSslContext(GetCertPath(), GetKeyPath(), GetCaPath());
		} catch (const std::exception&) {
			Log(LogWarning, "ElasticsearchWriter")
				<< "Unable to create SSL context.";
			throw;
		}

		stream.first = Shared<AsioTlsStream>::Make(IoEngine::Get().GetIoContext(), *sslContext, GetHost());

	} else {
		stream.second = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());
	}

	try {
		icinga::Connect(tls ? stream.first->lowest_layer() : stream.second->lowest_layer(), GetHost(), GetPort());
	} catch (const std::exception&) {
		Log(LogWarning, "ElasticsearchWriter")
			<< "Can't connect to Elasticsearch on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw;
	}

	if (tls) {
		auto& tlsStream (stream.first->next_layer());

		try {
			tlsStream.handshake(tlsStream.client);
		} catch (const std::exception&) {
			Log(LogWarning, "ElasticsearchWriter")
				<< "TLS handshake with host '" << GetHost() << "' on port " << GetPort() << " failed.";
			throw;
		}
	}

	return std::move(stream);
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
	auto milliSeconds = static_cast<int>((ts - static_cast<int>(ts)) * 1000);

	return Utility::FormatDateTime("%Y-%m-%dT%H:%M:%S", ts) + "." + Convert::ToString(milliSeconds) + Utility::FormatDateTime("%z", ts);
}
