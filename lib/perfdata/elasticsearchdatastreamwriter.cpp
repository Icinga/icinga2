/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "base/application.hpp"
#include "base/base64.hpp"
#include "base/defer.hpp"
#include "base/dictionary.hpp"
#include "base/exception.hpp"
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"
#include "base/stream.hpp"
#include "base/string.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsstream.hpp"
#include "base/utility.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/service.hpp"
#include "remote/url.hpp"

#include "perfdata/elasticsearchdatastreamwriter.hpp"
#include "perfdata/elasticsearchdatastreamwriter-ti.cpp"

using namespace icinga;

REGISTER_TYPE(ElasticsearchDatastreamWriter);

REGISTER_STATSFUNCTION(ElasticsearchDatastreamWriter, &ElasticsearchDatastreamWriter::StatsFunc);

void ElasticsearchDatastreamWriter::OnConfigLoaded()
{
	ObjectImpl<ElasticsearchDatastreamWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("ElasticsearchDatastreamWriter, " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, "ElasticsearchDatastreamWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void ElasticsearchDatastreamWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	for (const ElasticsearchDatastreamWriter::Ptr& writer : ConfigType::GetObjectsByType<ElasticsearchDatastreamWriter>()) {
		status->Set(
			writer->GetName(),
			new Dictionary({
				{ "work_queue_items", writer->m_WorkQueue.GetTaskCount(60) / 60.0 },
				{ "work_queue_item_rate", writer->m_WorkQueue.GetLength() },
				{ "documents_sent", writer->m_DocumentsSent },
				{ "documents_sent_error", writer->m_DocumentsFailed },
				{ "checkresults_filtered_out", writer->m_ItemsFilteredOut.load() }
			})
		);
	}
}

void ElasticsearchDatastreamWriter::Resume()
{
	ObjectImpl<ElasticsearchDatastreamWriter>::Resume();

	Log(LogInformation, "ElasticsearchDatastreamWriter")
		<< "'" << GetName() << "' resumed.";
	m_Paused = false;
	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	if (GetManageIndexTemplate()) {
		// Ensure index template exists/is updated as the first item on the work queue.
		m_WorkQueue.Enqueue([this]() { ManageIndexTemplate(); });
	}

	/* Setup timer for periodically flushing m_DataBuffer */
	m_FlushTimer = Timer::Create();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect([this](const Timer * const&) {
		m_WorkQueue.Enqueue([this]() { Flush(); });
	});
	m_FlushTimer->Start();

	/* Register for new metrics. */
	m_HandleCheckResults = Checkable::OnNewCheckResult.connect(
		[this](const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
			CheckResultHandler(checkable, cr);
		}
	);
}

/* Pause is equivalent to Stop, but with HA capabilities to resume at runtime. */
void ElasticsearchDatastreamWriter::Pause()
{
	m_HandleCheckResults.disconnect();
	m_FlushTimer->Stop(true);
	m_Paused = true;
	m_WorkQueue.Enqueue([this]() { Flush(); });
	m_WorkQueue.Join();

	Log(LogInformation, "ElasticsearchDatastreamWriter")
		<< "'" << GetName() << "' paused.";

	ObjectImpl<ElasticsearchDatastreamWriter>::Pause();
}

void ElasticsearchDatastreamWriter::ManageIndexTemplate() {
	AssertOnWorkQueue();

	String template_path = ICINGA_PKGDATADIR "/elasticsearch/index-template.json";
	std::ifstream template_file{ template_path, std::ifstream::in };
	if (!template_file.is_open()) {
		Log(LogCritical, "ElasticsearchDatastreamWriter")
			<< "Could not open index template file: " << template_path;
		return;
	}
	std::ostringstream template_stream;
	template_stream << template_file.rdbuf();
	String template_json = template_stream.str();
	template_file.close();
	Log(LogDebug, "ElasticsearchDatastreamWriter")
		<< "Read index template from " << template_path;
	Log(LogDebug, "ElasticsearchDatastreamWriter") << template_json;

	Url::Ptr index_template_url = new Url();
	index_template_url->SetScheme(GetEnableTls() ? "https" : "http");
	index_template_url->SetHost(GetHost());
	index_template_url->SetPort(GetPort());
	index_template_url->SetPath({ "_index_template", "icinga2-metrics" });

	Url::Ptr component_template_url = new Url();
	component_template_url->SetScheme(GetEnableTls() ? "https" : "http");
	component_template_url->SetHost(GetHost());
	component_template_url->SetPort(GetPort());
	component_template_url->SetPath({ "_component_template", "metrics-icinga2@custom" });
	component_template_url->SetQuery({ {"create", "true"} });

	while (true) {
		if (m_Paused) {
			Log(LogInformation, "ElasticsearchDatastreamWriter")
				<< "Shutdown in progress, aborting index template management.";
			return;
		}

		try {
			Dictionary::Ptr jsonResponse = TrySend(component_template_url, "{\"template\":{}}");
			Log(LogInformation, "ElasticsearchDatastreamWriter")
				<< "Successfully installed component template 'icinga2@custom'.";
		} catch (const StatusCodeException es) {
		  int status_code = es.GetStatusCode();
		  if (status_code == 400) {
			Log(LogInformation, "ElasticsearchDatastreamWriter")
				<< "Component template 'metrics-icinga2@custom' already exists, skipping creation.";
			// Continue to install/update index template
		  } else {
			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "Failed to install component template 'icinga2@custom', retrying in 5 seconds: " << DiagnosticInformation(es, false);
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Additional information:\n" << DiagnosticInformation(es, true);
			Utility::Sleep(5);
			continue;
		  }
		} catch (const std::exception& ex) {
			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "Failed to install component template 'icinga2@custom', retrying in 5 seconds: " << DiagnosticInformation(ex, false);
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Additional information:\n" << DiagnosticInformation(ex, true);
			Utility::Sleep(5);
			continue;
		}

		try {
			Dictionary::Ptr jsonResponse = TrySend(index_template_url, template_json);
			Log(LogInformation, "ElasticsearchDatastreamWriter")
				<< "Successfully installed/updated index template 'icinga2-metrics'.";
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Response: " << JsonEncode(jsonResponse);
			break;
		} catch (const std::exception& ex) {
			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "Failed to install/update index template 'icinga2-metrics', retrying in 5 seconds: " << DiagnosticInformation(ex, false);
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Additional information:\n" << DiagnosticInformation(ex, true);
			Utility::Sleep(5);
		}
	}
}

Dictionary::Ptr ElasticsearchDatastreamWriter::ExtractPerfData(const Checkable::Ptr checkable, const Array::Ptr& perfdata) {
	Dictionary::Ptr pd_fields = new Dictionary();
	if (!perfdata)
		return pd_fields;

	ObjectLock olock(perfdata);
	for (const Value& val : perfdata) {
		PerfdataValue::Ptr pdv;

		if (val.IsObjectType<PerfdataValue>())
			pdv = val;
		else {
			try {
				pdv = PerfdataValue::Parse(val);
			} catch (const std::exception&) {
				Log(LogWarning, "ElasticsearchDatastreamWriter")
					<< "Ignoring invalid perfdata for checkable '"
					<< checkable->GetName() << "' with value: " << val;
				continue;
			}
		}

		Dictionary::Ptr metric = new Dictionary();
		metric->Set("value", pdv->GetValue());
		metric->Set("counter", pdv->GetCounter());

		if (!pdv->GetMin().IsEmpty()) metric->Set("min", pdv->GetMin());
		if (!pdv->GetMax().IsEmpty()) metric->Set("max", pdv->GetMax());
		if (!pdv->GetUnit().IsEmpty()) metric->Set("unit", pdv->GetUnit());

		if (!pdv->GetWarn().IsEmpty() && GetEnableSendThresholds())
			metric->Set("warn", pdv->GetWarn());
		if (!pdv->GetCrit().IsEmpty() && GetEnableSendThresholds())
			metric->Set("crit", pdv->GetCrit());

		pd_fields->Set(pdv->GetLabel(), metric);
	}

	return pd_fields;
}

// user-defined tags, as specified in the ECS specification: https://www.elastic.co/docs/reference/ecs/ecs-base#field-tags
Array::Ptr ElasticsearchDatastreamWriter::ExtractTemplateTags(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr) {
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Array::Ptr tag_tmpl = service ? GetServiceTagsTemplate() : GetHostTagsTemplate();
	if (tag_tmpl == nullptr) {
		return nullptr;
	}

	MacroProcessor::ResolverList resolvers;
	resolvers.emplace_back("host", host);
	if (service) {
		resolvers.emplace_back("service", service);
	}

	ObjectLock olock(tag_tmpl);
	Array::Ptr tags = new Array();
	for (const String tag : tag_tmpl) {
		String missingMacro;
		Value value = MacroProcessor::ResolveMacros(tag, resolvers, cr, &missingMacro);
		if (!missingMacro.IsEmpty()) {
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Missing macro for tag: " << tag
				<< ". Missing: " << missingMacro
				<< " for checkable '" << checkable->GetName() << "'. Skipping.";
			continue;
		}
		tags->Add(value);
	}

	return tags;
}

// user-defined labels, as specified in the ECS specification: https://www.elastic.co/docs/reference/ecs/ecs-base#field-labels
Dictionary::Ptr ElasticsearchDatastreamWriter::ExtractTemplateLabels(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr) {
	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr labels_tmpl = service ? GetServiceLabelsTemplate() : GetHostLabelsTemplate();
	if (labels_tmpl == nullptr) {
		return nullptr;
	}

	MacroProcessor::ResolverList resolvers;
	resolvers.emplace_back("host", host);
	if (service) {
		resolvers.emplace_back("service", service);
	}

	ObjectLock olock(labels_tmpl);
	Dictionary::Ptr labels = new Dictionary();
	for (const Dictionary::Pair& label_kv : labels_tmpl) {
		String missingMacro;
		Value value = MacroProcessor::ResolveMacros(label_kv.second, resolvers, cr, &missingMacro);
		if (!missingMacro.IsEmpty()) {
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Missing macro for label: " << label_kv.first
				<< "Label: " << label_kv.second
				<< ". Missing: " << missingMacro
				<< " for checkable '" << checkable->GetName() << "'. Skipping.";
			continue;
		}
		labels->Set(label_kv.first, value);
	}

	return labels;
}

bool ElasticsearchDatastreamWriter::Filter(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr) {
	if (!m_CompiledFilter)
		return true;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Namespace::Ptr frameNS = new Namespace();
	frameNS->Set("checkable", checkable);
	frameNS->Set("check_result", cr);
	frameNS->Set("host", host);
	frameNS->Set("service", service);

	ScriptFrame frame(true, frameNS);
	return Convert::ToBool(m_CompiledFilter->Evaluate(frame));
}

void ElasticsearchDatastreamWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	if (!Filter(checkable, cr)) {
		m_ItemsFilteredOut.fetch_add(1);
		Log(LogDebug, "ElasticsearchDatastreamWriter")
			<< "Check result for checkable '" << checkable->GetName() << "' filtered out.";
		return;
	}

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr ecs_metadata = new Dictionary({
		{ "version", "8.0.0" }
	});

	String datastream_name = "metrics-icinga2." + checkable->GetCheckCommandRaw() + "-" + GetDatastreamNamespace();
	Dictionary::Ptr data_stream = new Dictionary({
		{"type", "metrics"},
		{"dataset", "icinga2." + checkable->GetCheckCommandRaw()},
		{"namespace", GetDatastreamNamespace()}
	});

	Dictionary::Ptr ecs_host = new Dictionary({
		{"name", host->GetDisplayName()},
		{"hostname", host->GetName()},
		{"zone", host->GetZone()->GetZoneName()},
		{"soft_state", host->GetState()},
		{"hard_state", host->GetLastHardState()}
	});

	char _addr[16];
	if (!host->GetAddress().IsEmpty() && inet_pton(AF_INET, host->GetAddress().CStr(), _addr) == 1) {
		ecs_host->Set("ip", host->GetAddress());
	} else if (!host->GetAddress6().IsEmpty() && inet_pton(AF_INET6, host->GetAddress6().CStr(), _addr) == 1) {
		ecs_host->Set("ip", host->GetAddress6());
	} else if (!host->GetAddress().IsEmpty()) {
		ecs_host->Set("fqdn", host->GetAddress());
	}

	Dictionary::Ptr ecs_service;
	if (service) {
	   	ecs_service = new Dictionary({
	   		{"name", service->GetName()},
			{"display_name", service->GetDisplayName()},
			{"zone", host->GetZone()->GetZoneName()},
			{"soft_state", service->GetState()},
			{"hard_state", service->GetLastHardState()}
		});
	}

	Dictionary::Ptr ecs_agent = new Dictionary({
		{"type", "icinga2"},
		{"name", cr->GetCheckSource()}
	});
	Endpoint::Ptr endpoint = checkable->GetCommandEndpoint();
	if (endpoint) {
		ecs_agent->Set("id", endpoint->GetName());
		ecs_agent->Set("version", FormatIcingaVersion(endpoint->GetIcingaVersion()));
	} else {
		ecs_agent->Set("id", IcingaApplication::GetInstance()->GetName());
		ecs_agent->Set("version", FormatIcingaVersion((unsigned long) IcingaApplication::GetInstance()->GetVersion()));
	}

	Dictionary::Ptr ecs_event = new Dictionary({
	   {"created", FormatTimestamp(cr->GetScheduleEnd())},
	   {"start", FormatTimestamp(cr->GetExecutionStart())},
	   {"end", FormatTimestamp(cr->GetExecutionEnd())},
	   {"kind", "metric"},
	   {"module", "icinga2"}
	});

	String checkable_name = service == nullptr ? host->GetName() : service->GetName();
	Dictionary::Ptr check_result = new Dictionary({
		{"checkable", checkable_name},
		{"exit_status", cr->GetExitStatus()},
		{"execution_time", cr->CalculateExecutionTime()},
		{"latency", cr->CalculateLatency()},
		{"schedule_start", FormatTimestamp(cr->GetScheduleStart())},
		{"schedule_end", FormatTimestamp(cr->GetScheduleEnd())},
		{"active", cr->GetActive()}
	});

	Dictionary::Ptr perf_data = ExtractPerfData(checkable, cr->GetPerformanceData());

	Dictionary::Ptr document = new Dictionary({
		{"@timestamp", FormatTimestamp(cr->GetScheduleEnd())},
		{ "ecs", ecs_metadata },
		{ "data_stream", data_stream },
		{ "host", ecs_host },
		{ "service", ecs_service },
		{ "agent", ecs_agent },
		{ "event", ecs_event },
		{ "check", check_result },
		{ "message", cr->GetOutput() },
		{ "perf_data", perf_data },
		{ "tags", ExtractTemplateTags(checkable, cr) },
		{ "labels", ExtractTemplateLabels(checkable, cr) }
	});

	EcsDocument::Ptr workqueue_document = new EcsDocument(
		datastream_name,
		document
	);

	//ToDo: This will block on a full queue. Consider a drop policy.
	m_WorkQueue.Enqueue([this, workqueue_document, checkable]() {
		m_DataBuffer.emplace_back(workqueue_document);
		if (static_cast<int>(m_DataBuffer.size()) >= GetFlushThreshold()) {
			Flush();
		}
	});
}

void ElasticsearchDatastreamWriter::Flush()
{
	AssertOnWorkQueue();

	/* Flush can be called from 1) Timeout 2) Threshold 3) on shutdown/reload. */
	if (m_DataBuffer.empty())
		return;

	Url::Ptr url = new Url();
	url->SetScheme(GetEnableTls() ? "https" : "http");
	url->SetHost(GetHost());
	url->SetPort(GetPort());
	url->SetPath({ "_bulk" });

	String body = String();
	for (const auto &document : m_DataBuffer) {
		Dictionary::Ptr index = new Dictionary({
			{ "create", new Dictionary({ { "_index", document->GetIndex() } }) }
		});

		body += JsonEncode(index) + "\n";
		body += JsonEncode(document->GetDocument()) + "\n";
	}

	Dictionary::Ptr jsonResponse;
	while (true) {
		try {
			jsonResponse = TrySend(url, body);
			m_DocumentsSent += m_DataBuffer.size();
			break;
		} catch (const std::exception& ex) {
			if (m_Paused) {
				// We are shutting down, don't retry.
				Log(LogWarning, "ElasticsearchDatastreamWriter")
					<< "Flush failed during shutdown, dropping " << m_DataBuffer.size() << " documents: " << DiagnosticInformation(ex, false);
				m_DataBuffer.clear();
				break;
			}

			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "Flush failed, retrying in 5 seconds: " << DiagnosticInformation(ex, false);
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Additional information:\n" << DiagnosticInformation(ex, true);
			Utility::Sleep(5);
		}
	}

	Log(LogInformation, "ElasticsearchDatastreamWriter")
		<< "Successfully sent " << m_DataBuffer.size()
		<< " documents to Elasticsearch. Took "
		<< jsonResponse->Get("took") << "ms.";

	Value errors = jsonResponse->Get("errors");
	if (!errors.ToBool()) {
		Log(LogDebug, "ElasticsearchDatastreamWriter") << "No errors during write operation.";
		m_DataBuffer.clear();
		return;
	}

	Array::Ptr items = jsonResponse->Get("items");
	int c = 0;
	ObjectLock olock(items);
	for (const Dictionary::Ptr value : items) {
		Dictionary::Ptr item_result = value->Get("create");
		int item_result_status = item_result->Get("status");
		if (item_result_status > 299) {
			m_DocumentsFailed += 1;
			Dictionary::Ptr item_error = item_result->Get("error");
			String value = item_error->Get("type") + ": " + item_error->Get("reason");
			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "Error during document creation: " << value;
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Error response: " << JsonEncode(item_result) << "\n"
				<< "Document: " << JsonEncode(m_DataBuffer[c]->GetDocument());
		}
		++c;
	}
	m_DataBuffer.clear();
}

Value ElasticsearchDatastreamWriter::TrySend(Url::Ptr url, String body) {
	// Make sure we are not randomly flushing from a timeout under load.
	m_FlushTimer->Reschedule(-1);

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
	String api_token = GetPassword();

	if (!username.IsEmpty() && !password.IsEmpty()) {
		request.set(http::field::authorization, "Basic " + Base64::Encode(username + ":" + password));
	} else if (!api_token.IsEmpty()) {
		request.set(http::field::authorization, "ApiKey " + api_token);
	}

	request.body() = body;
	request.content_length(request.body().size());

	/* Don't log the request body to debug log, this is already done above. */
	Log(LogDebug, "ElasticsearchDatastreamWriter")
		<< "Sending " << request.method_string() << " request" << ((!username.IsEmpty() && !password.IsEmpty()) ? " with basic auth" : "" )
		<< " to '" << url->Format() << "'.";

	http::parser<false, http::string_body> parser;
	beast::flat_buffer buf;

	OptionalTlsStream stream = Connect();
	Defer closeStream([&stream]() {
		if (stream.first) {
			stream.first->lowest_layer().close();
		} else if (stream.second) {
			stream.second->lowest_layer().close();
		}
	});

	try {
		if (stream.first) {
			http::write(*stream.first, request);
			stream.first->flush();
			http::read(*stream.first, buf, parser);
		} else {
			http::write(*stream.second, request);
			stream.second->flush();
			http::read(*stream.second, buf, parser);
		}
	} catch (const std::exception&) {
		Log(LogWarning, "ElasticsearchDatastreamWriter")
			<< "Cannot perform http request API on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw;
	}

	auto& response (parser.get());
	if (response.result_int() > 299) {
		Log(LogCritical, "ElasticsearchDatastreamWriter")
			<< "Unexpected response code " << response.result_int() << " from URL '" << url->Format() << "'. Error: " << response.body();
		BOOST_THROW_EXCEPTION(StatusCodeException(
			response.result_int(),
			"Unexpected response code from Elasticsearch",
			response.body()
		));
	}

	m_DocumentsSent += m_DataBuffer.size();
	auto& contentType (response[http::field::content_type]);
	if (contentType != "application/json" && contentType != "application/json; charset=utf-8") {
		Log(LogCritical, "ElasticsearchDatastreamWriter") << "Unexpected Content-Type: '" << contentType << "'";
		BOOST_THROW_EXCEPTION(std::runtime_error(String("Unexpected Content-Type")));
	}

	auto& response_body (response.body());

	Dictionary::Ptr jsonResponse;
	try {
		return JsonDecode(response_body);
	} catch (...) {
		Log(LogWarning, "ElasticsearchDatastreamWriter")
			<< "Unable to parse JSON response:\n" << body;
		throw;
	}
}

OptionalTlsStream ElasticsearchDatastreamWriter::Connect()
{
	Log(LogNotice, "ElasticsearchDatastreamWriter")
		<< "Connecting to Elasticsearch on host '" << GetHost() << "' port '" << GetPort() << "'.";

	OptionalTlsStream stream;
	bool tls = GetEnableTls();

	if (tls) {
		Shared<boost::asio::ssl::context>::Ptr sslContext;

		try {
			sslContext = MakeAsioSslContext(GetCertPath(), GetKeyPath(), GetCaPath());
		} catch (const std::exception&) {
			Log(LogWarning, "ElasticsearchDatastreamWriter")
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
		Log(LogWarning, "ElasticsearchDatastreamWriter")
			<< "Can't connect to Elasticsearch on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw;
	}

	if (tls) {
		auto& tlsStream (stream.first->next_layer());

		try {
			tlsStream.handshake(tlsStream.client);
		} catch (const std::exception&) {
			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "TLS handshake with host '" << GetHost() << "' on port " << GetPort() << " failed.";
			throw;
		}

		if (!GetInsecureNoverify()) {
			if (!tlsStream.GetPeerCertificate()) {
				BOOST_THROW_EXCEPTION(std::runtime_error("Elasticsearch didn't present any TLS certificate."));
			}

			if (!tlsStream.IsVerifyOK()) {
				BOOST_THROW_EXCEPTION(std::runtime_error(
					"TLS certificate validation failed: " + std::string(tlsStream.GetVerifyError())
				));
			}
		}
	}

	return stream;
}


void ElasticsearchDatastreamWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void ElasticsearchDatastreamWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "ElasticsearchDatastreamWriter", "Exception during Elastic operation: Verify that your backend is operational!");

	Log(LogDebug, "ElasticsearchDatastreamWriter")
		<< "Exception during Elasticsearch operation: " << DiagnosticInformation(std::move(exp));
}

String ElasticsearchDatastreamWriter::FormatTimestamp(double ts)
{
	/* The date format must match the default dynamic date detection
	 * pattern in indexes. This enables applications like Kibana to
	 * detect a qualified timestamp index for time-series data.
	 *
	 * Example: 2017-09-11T10:56:21.463+0200
	 *
	 * References:
	 * https://www.elastic.co/guide/en/elasticsearchdatastream/reference/current/dynamic-field-mapping.html#date-detection
	 * https://www.elastic.co/guide/en/elasticsearchdatastream/reference/current/mapping-date-format.html
	 * https://www.elastic.co/guide/en/elasticsearchdatastream/reference/current/date.html
	 */
	auto milliSeconds = static_cast<int>((ts - static_cast<int>(ts)) * 1000);

	return Utility::FormatDateTime("%Y-%m-%dT%H:%M:%S", ts) + "." + Convert::ToString(milliSeconds) + Utility::FormatDateTime("%z", ts);
}

String ElasticsearchDatastreamWriter::FormatIcingaVersion(unsigned long version) {
	auto bugfix = version % 100;
	version /= 100;
	auto minor = version % 100;
	auto major = version / 100;
	return String() + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(bugfix);
}


void ElasticsearchDatastreamWriter::ValidateTagsTemplate(Array::Ptr tags) {
	ObjectLock olock(tags);
	for (const Value& tag : tags) {
		if (!MacroProcessor::ValidateMacroString(tag)) {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "host_tags_template" }, "Closing $ not found in macro format string '" + tag + "'."));
		}
	}
}

void ElasticsearchDatastreamWriter::ValidateHostTagsTemplate(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ElasticsearchDatastreamWriter>::ValidateHostTagsTemplate(lvalue, utils);
	Array::Ptr tags = lvalue();
	if (tags) {
		ValidateTagsTemplate(tags);
	}
}

void ElasticsearchDatastreamWriter::ValidateServiceTagsTemplate(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ElasticsearchDatastreamWriter>::ValidateServiceTagsTemplate(lvalue, utils);
	Array::Ptr tags = lvalue();
	if (tags) {
		ValidateTagsTemplate(tags);
	}
}

void ElasticsearchDatastreamWriter::ValidateLabelsTemplate(Dictionary::Ptr labels) {
	ObjectLock olock(labels);
	for (const Dictionary::Pair& kv : labels) {
		if (!MacroProcessor::ValidateMacroString(kv.second)) {
			BOOST_THROW_EXCEPTION(ValidationError(this, { "host_tags_template", kv.first }, "Closing $ not found in macro format string '" + kv.second + "'."));
		}
	}
}

void ElasticsearchDatastreamWriter::ValidateHostLabelsTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ElasticsearchDatastreamWriter>::ValidateHostLabelsTemplate(lvalue, utils);
	Dictionary::Ptr labels = lvalue();
	if (labels) {
		ValidateLabelsTemplate(labels);
	}
}

void ElasticsearchDatastreamWriter::ValidateServiceLabelsTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ElasticsearchDatastreamWriter>::ValidateServiceLabelsTemplate(lvalue, utils);
	Dictionary::Ptr labels = lvalue();
	if (labels) {
		ValidateLabelsTemplate(labels);
	}
}

void ElasticsearchDatastreamWriter::ValidateFilter(const Lazy<Value> &lvalue, const ValidationUtils &utils) {
	Value filter = lvalue();
	if (filter.IsEmpty()) {
		return;
	}

	if (!filter.IsObjectType<Function>()) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "filter" }, "Filter must be an expression."));
	}


	std::vector<std::unique_ptr<Expression> > args;
	args.emplace_back(new GetScopeExpression(ScopeThis));
	std::unique_ptr<Expression> indexer{new IndexerExpression(
	std::unique_ptr<Expression>(MakeLiteral(filter)),
	std::unique_ptr<Expression>(MakeLiteral("call"))
	)};
	FunctionCallExpression *fexpr = new FunctionCallExpression(std::move(indexer), std::move(args));

	m_CompiledFilter = std::move(fexpr);
}

EcsDocument::EcsDocument(String index, Dictionary::Ptr document) {
	SetIndex(index, true);
	SetDocument(document, true);
}
