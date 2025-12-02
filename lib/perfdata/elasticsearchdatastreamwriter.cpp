/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
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
#include "base/logger.hpp"
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
namespace beast = boost::beast;
namespace http = beast::http;

static void ExceptionHandler(const boost::exception_ptr& exp);
static Array::Ptr ExtractTemplateTags(const MacroProcessor::ResolverList& resolvers, const Array::Ptr& tagsTmpl,
	const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
static Dictionary::Ptr ExtractTemplateLabels(const MacroProcessor::ResolverList& resolvers, const Dictionary::Ptr& labelsTmpl,
	const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
static String NormalizeElasticsearchFieldName(const String& fieldName);
static String NormalizeElasticsearchIndexPart(const String& fieldName);

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

void ElasticsearchDatastreamWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
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
	m_WorkQueue.SetExceptionCallback([](boost::exception_ptr exp) { ExceptionHandler(exp); });

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

void ElasticsearchDatastreamWriter::ManageIndexTemplate()
{
	AssertOnWorkQueue();

	String templatePath = ICINGA_PKGDATADIR "/elasticsearch/index-template.json";
	std::ifstream templateFile{ templatePath, std::ifstream::in };
	if (!templateFile.is_open()) {
		Log(LogCritical, "ElasticsearchDatastreamWriter")
			<< "Could not open index template file: " << templatePath;
		return;
	}
	std::ostringstream templateStream;
	templateStream << templateFile.rdbuf();
	String templateJson = templateStream.str();
	templateFile.close();
	Log(LogDebug, "ElasticsearchDatastreamWriter")
		<< "Read index template from " << templatePath;
	Log(LogDebug, "ElasticsearchDatastreamWriter") << templateJson;

	Url::Ptr indexTemplateUrl = new Url();
	indexTemplateUrl->SetScheme(GetEnableTls() ? "https" : "http");
	indexTemplateUrl->SetHost(GetHost());
	indexTemplateUrl->SetPort(GetPort());
	indexTemplateUrl->SetPath({ "_index_template", "icinga2-metrics" });

	Url::Ptr componentTemplateUrl = new Url();
	componentTemplateUrl->SetScheme(GetEnableTls() ? "https" : "http");
	componentTemplateUrl->SetHost(GetHost());
	componentTemplateUrl->SetPort(GetPort());
	componentTemplateUrl->SetPath({ "_component_template", "metrics-icinga2@custom" });
	componentTemplateUrl->SetQuery({ {"create", "true"} });

	while (true) {
		if (m_Paused) {
			Log(LogInformation, "ElasticsearchDatastreamWriter")
				<< "Shutdown in progress, aborting index template management.";
			return;
		}

		try {
			Dictionary::Ptr jsonResponse = TrySend(componentTemplateUrl, "{\"template\":{}}");
			Log(LogInformation, "ElasticsearchDatastreamWriter")
				<< "Successfully installed component template 'icinga2@custom'.";
		} catch (const StatusCodeException& es) {
			if (es.GetStatusCode() == http::status::bad_request) {
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
		    // don't move, as we retry in a loop if it fails.
			Dictionary::Ptr jsonResponse = TrySend(indexTemplateUrl, templateJson);
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

Dictionary::Ptr ElasticsearchDatastreamWriter::ExtractPerfData(const Checkable::Ptr& checkable, const Array::Ptr& perfdata)
{
	Dictionary::Ptr pdFields = new Dictionary();
	if (!perfdata)
		return pdFields;

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
		if (pdv->GetCounter()) metric->Set("counter", pdv->GetCounter());

		if (!pdv->GetMin().IsEmpty()) metric->Set("min", pdv->GetMin());
		if (!pdv->GetMax().IsEmpty()) metric->Set("max", pdv->GetMax());
		if (!pdv->GetUnit().IsEmpty()) metric->Set("unit", pdv->GetUnit());

		if (!pdv->GetWarn().IsEmpty() && GetEnableSendThresholds())
			metric->Set("warn", pdv->GetWarn());
		if (!pdv->GetCrit().IsEmpty() && GetEnableSendThresholds())
			metric->Set("crit", pdv->GetCrit());

		String label = NormalizeElasticsearchFieldName(pdv->GetLabel());
		pdFields->Set(label, metric);
	}

	return pdFields;
}

// user-defined tags, as specified in the ECS specification: https://www.elastic.co/docs/reference/ecs/ecs-base#field-tags
static Array::Ptr ExtractTemplateTags(const MacroProcessor::ResolverList& resolvers,
	const Array::Ptr& tagsTmpl, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (tagsTmpl == nullptr) {
		return nullptr;
	}

	ObjectLock olock(tagsTmpl);
	Array::Ptr tags = new Array();
	for (const String tag : tagsTmpl) {
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
static Dictionary::Ptr ExtractTemplateLabels(const MacroProcessor::ResolverList& resolvers,
	const Dictionary::Ptr& labelsTmpl, const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (labelsTmpl == nullptr) {
		return nullptr;
	}

	ObjectLock olock(labelsTmpl);
	Dictionary::Ptr labels = new Dictionary();
	for (const Dictionary::Pair& labelKv : labelsTmpl) {
		String missingMacro;
		Value value = MacroProcessor::ResolveMacros(labelKv.second, resolvers, cr, &missingMacro);
		if (!missingMacro.IsEmpty()) {
			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Missing macro for label: " << labelKv.first
				<< "Label: " << labelKv.second
				<< ". Missing: " << missingMacro
				<< " for checkable '" << checkable->GetName() << "'. Skipping.";
			continue;
		}
		labels->Set(labelKv.first, value);
	}

	return labels;
}

String ElasticsearchDatastreamWriter::ExtractDatastreamNamespace(const MacroProcessor::ResolverList& resolvers,
	const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	String namespaceTmpl = GetDatastreamNamespace();

	String missingMacro;
	Value value = MacroProcessor::ResolveMacros(namespaceTmpl, resolvers, cr, &missingMacro);
	if (!missingMacro.IsEmpty() || value.IsEmpty()) {
		Log(LogDebug, "ElasticsearchDatastreamWriter")
			<< "Missing value for namespace. Missing: " << missingMacro
			<< " for checkable '" << checkable->GetName() << "'. Skipping.";
		return "default";
	}

	return NormalizeElasticsearchIndexPart(value);
}

bool ElasticsearchDatastreamWriter::Filter(const Checkable::Ptr& checkable)
{
	if (!m_CompiledFilter)
		return true;

	auto [host, service] = GetHostService(checkable);

	Namespace::Ptr frameNS = new Namespace();
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

	if (!Filter(checkable)) {
		m_ItemsFilteredOut.fetch_add(1);
		Log(LogDebug, "ElasticsearchDatastreamWriter")
			<< "Check result for checkable '" << checkable->GetName() << "' filtered out.";
		return;
	}

	auto [host, service] = GetHostService(checkable);

	/* ECS host object population
	 * References:
	 *   https://www.elastic.co/guide/en/ecs/current/ecs-host.html
	 * The classic ECS host fields (name, hostname) are extended here with
	 * Icinga-specific state information (soft_state, hard_state) to aid consumers
	 * correlating monitoring state with metrics.
	 */
	Dictionary::Ptr ecsHost = new Dictionary({
		{"name", host->GetDisplayName()},
		{"hostname", host->GetName()},
		{"soft_state", host->GetState()},
		{"hard_state", host->GetLastHardState()}
	});
	if (!host->GetZoneName().IsEmpty()) ecsHost->Set("zone", host->GetZoneName());

	Dictionary::Ptr ecsService;
	if (service) {
		/* ECS service object population
		 * References:
		 *   https://www.elastic.co/guide/en/ecs/current/ecs-service.html
		 * We include ECS service 'name' plus Icinga display/state details.
		 * The added state fields (soft_state, hard_state) are Icinga-centric
		 * extensions.
		 */
		ecsService = new Dictionary({
			{"name", service->GetName()},
			{"display_name", service->GetDisplayName()},
			{"soft_state", service->GetState()},
			{"hard_state", service->GetLastHardState()}
		});
		if (!service->GetZoneName().IsEmpty()) ecsService->Set("zone", service->GetZoneName());
	}


	Dictionary::Ptr checkResult = new Dictionary({
		{"current_check_attempts", checkable->GetCheckAttempt()},
		{"reachable", checkable->IsReachable()}
	});

	m_WorkQueue.Enqueue([this, cr, checkable, host = std::move(host), service = std::move(service), ecsHost = std::move(ecsHost),
		ecsService = std::move(ecsService), checkResult = std::move(checkResult)]()
	{
		MacroProcessor::ResolverList resolvers;
		resolvers.emplace_back("host", host);
		if (service) {
			resolvers.emplace_back("service", service);
		}

		Dictionary::Ptr ecs_metadata = new Dictionary({
			{ "version", "8.0.0" }
		});

		String datastreamNamespace = ExtractDatastreamNamespace(resolvers, checkable, cr);
		String datastreamDataset = "icinga2." + NormalizeElasticsearchIndexPart(checkable->GetCheckCommandRaw());
		String datastreamName = "metrics-" + datastreamDataset + "-" + datastreamNamespace;
		Dictionary::Ptr data_stream = new Dictionary({
			{"type", "metrics"},
			{"dataset", datastreamDataset},
			{"namespace", datastreamNamespace}
		});

		char _addr[16];
		if (!host->GetAddress().IsEmpty() && inet_pton(AF_INET, host->GetAddress().CStr(), _addr) == 1) {
			ecsHost->Set("ip", host->GetAddress());
		} else if (!host->GetAddress6().IsEmpty() && inet_pton(AF_INET6, host->GetAddress6().CStr(), _addr) == 1) {
			ecsHost->Set("ip", host->GetAddress6());
		} else if (!host->GetAddress().IsEmpty()) {
			ecsHost->Set("fqdn", host->GetAddress());
		}

		Dictionary::Ptr ecsAgent = new Dictionary({
			{"type", "icinga2"},
			{"name", cr->GetCheckSource()}
		});
		Endpoint::Ptr endpoint = checkable->GetCommandEndpoint();
		if (endpoint) {
			ecsAgent->Set("id", endpoint->GetName());
			ecsAgent->Set("version", endpoint->GetIcingaVersionString());
		} else {
			ecsAgent->Set("id", IcingaApplication::GetInstance()->GetName());
			ecsAgent->Set("version", IcingaApplication::GetAppSpecVersion());
		}

		Dictionary::Ptr ecsEvent = new Dictionary({
			{"created", FormatTimestamp(cr->GetScheduleEnd())},
			{"start", FormatTimestamp(cr->GetExecutionStart())},
			{"end", FormatTimestamp(cr->GetExecutionEnd())},
			{"kind", "metric"},
			{"module", "icinga2"}
		});

		checkResult->Set("exit_status", cr->GetExitStatus());
		checkResult->Set("execution_time", cr->CalculateExecutionTime());
		checkResult->Set("latency", cr->CalculateLatency());
		checkResult->Set("schedule_start", FormatTimestamp(cr->GetScheduleStart()));
		checkResult->Set("schedule_end", FormatTimestamp(cr->GetScheduleEnd()));
		checkResult->Set("active", cr->GetActive());
		checkResult->Set("max_attempts", checkable->GetMaxCheckAttempts());

		Dictionary::Ptr document = new Dictionary({
			{"@timestamp", FormatTimestamp(cr->GetExecutionEnd())},
			{ "ecs", ecs_metadata },
			{ "data_stream", data_stream },
			{ "host", ecsHost },
			{ "agent", ecsAgent },
			{ "event", ecsEvent },
			{ "check", checkResult },
			{ "message", cr->GetOutput() },
		});

		Dictionary::Ptr perfdata = ExtractPerfData(checkable, cr->GetPerformanceData());
		if (perfdata->GetLength() != 0) {
			document->Set("perfdata", perfdata);
		}

		Array::Ptr ecsTags = ExtractTemplateTags(
			resolvers, service ? GetServiceTagsTemplate() : GetHostTagsTemplate(), checkable, cr);
		if (ecsTags && ecsTags->GetLength() != 0 ) {
			document->Set("tags", ecsTags);
		}

		Dictionary::Ptr ecsLabels = ExtractTemplateLabels(
			resolvers, service ? GetServiceLabelsTemplate() : GetHostLabelsTemplate(), checkable, cr);
		if (ecsLabels && ecsLabels->GetLength() != 0 ) {
			document->Set("labels", ecsLabels);
		}

		if (ecsService && ecsService->GetLength() != 0 ) {
			document->Set("service", ecsService);
		}

		m_DataBuffer.emplace_back(new EcsDocument(datastreamName,document));
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
			jsonResponse = TrySend(url, std::move(body));
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
		Dictionary::Ptr itemResult = value->Get("create");
		int itemResultStatus = itemResult->Get("status");
		if (itemResultStatus > 299) {
			m_DocumentsFailed += 1;
			Dictionary::Ptr itemError = itemResult->Get("error");
			String value = itemError->Get("type") + ": " + itemError->Get("reason");
			Log(LogWarning, "ElasticsearchDatastreamWriter")
				<< "Error during document creation: " << value;

			if (c >= m_DataBuffer.size()) {
				Log(LogCritical, "ElasticsearchDatastreamWriter")
					<< "Got an error response from elasticsearch ouside the sent messages";
				break;
			}

			Log(LogDebug, "ElasticsearchDatastreamWriter")
				<< "Error response: " << JsonEncode(itemResult) << "\n"
				<< "Document: " << JsonEncode(m_DataBuffer[c]->GetDocument());
		}
		++c;
	}
	m_DataBuffer.clear();
}

Value ElasticsearchDatastreamWriter::TrySend(const Url::Ptr& url, String body)
{
	// Make sure we are not randomly flushing from a timeout under load.
	m_FlushTimer->Reschedule(-1);

	http::request<http::string_body> request (http::verb::post, std::string(url->Format(true)), 10);
	request.set(http::field::user_agent, "Icinga/" + Application::GetAppSpecVersion());
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
	String apiToken = GetApiToken();

	if (!username.IsEmpty() && !password.IsEmpty()) {
		request.set(http::field::authorization, "Basic " + Base64::Encode(username + ":" + password));
	} else if (!apiToken.IsEmpty()) {
		request.set(http::field::authorization, "ApiKey " + apiToken);
	}

	request.body() = std::move(body);
	request.content_length(request.body().size());

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
		Log(LogDebug, "ElasticsearchDatastreamWriter")
			<< "Unexpected response code " << static_cast<unsigned>(response.result()) << " from URL '" << url->Format() << "'. Error: " << response.body();
		BOOST_THROW_EXCEPTION(StatusCodeException(
			response.result(),
			"Unexpected response code from Elasticsearch",
			response.body()
		));
	}

	auto& contentType (response[http::field::content_type]);
	/* Accept application/json with optional charset (any variant), case-insensitive. */
	std::string ctLower = std::string(contentType.data(), contentType.size());
	boost::trim(ctLower);
	boost::to_lower(ctLower);
	if (!(ctLower == "application/json" || ctLower == "application/json; charset=utf-8")) {
		Log(LogCritical, "ElasticsearchDatastreamWriter") << "Unexpected Content-Type: '" << ctLower << "'";
		BOOST_THROW_EXCEPTION(std::runtime_error(String("Unexpected Content-Type '" + ctLower + "'")));
	}

	auto& responseBody (response.body());

	Dictionary::Ptr jsonResponse;
	try {
		return JsonDecode(responseBody);
	} catch (...) {
		Log(LogWarning, "ElasticsearchDatastreamWriter")
			<< "Unable to parse JSON response:\n" << responseBody;
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
			tlsStream.handshake(boost::asio::ssl::stream_base::client);
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

static void ExceptionHandler(const boost::exception_ptr& exp)
{
	Log(LogCritical, "ElasticsearchDatastreamWriter", "Exception during Elastic operation: Verify that your backend is operational!");

	Log(LogDebug, "ElasticsearchDatastreamWriter")
		<< "Exception during Elasticsearch operation: " << DiagnosticInformation(exp);
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

void ElasticsearchDatastreamWriter::ValidateTagsTemplate(const Array::Ptr& tags)
{
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
	auto& tags = lvalue();
	if (tags) {
		ValidateTagsTemplate(tags);
	}
}

void ElasticsearchDatastreamWriter::ValidateServiceTagsTemplate(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ElasticsearchDatastreamWriter>::ValidateServiceTagsTemplate(lvalue, utils);
	auto& tags = lvalue();
	if (tags) {
		ValidateTagsTemplate(tags);
	}
}

void ElasticsearchDatastreamWriter::ValidateLabelsTemplate(const Dictionary::Ptr& labels)
{
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
	auto& labels = lvalue();
	if (labels) {
		ValidateLabelsTemplate(labels);
	}
}

void ElasticsearchDatastreamWriter::ValidateServiceLabelsTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ElasticsearchDatastreamWriter>::ValidateServiceLabelsTemplate(lvalue, utils);
	auto& labels = lvalue();
	if (labels) {
		ValidateLabelsTemplate(labels);
	}
}

void ElasticsearchDatastreamWriter::ValidateFilter(const Lazy<Value> &lvalue, const ValidationUtils &)
{
	const Value &filter = lvalue();
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

	m_CompiledFilter = fexpr;
}

static String NormalizeElasticsearchIndexPart(const String& fieldName)
{
	String normalized("");

	bool first_char = true;
	bool preceding_invalid = false;
	for (char& c : boost::trim_copy(fieldName)) {
		if (!std::isalnum(static_cast<unsigned char>(c))) {
			if (first_char || preceding_invalid) continue;
			preceding_invalid = true;
			c = '_';
		} else {
			first_char = false;
			preceding_invalid = false;
		}

		normalized += tolower(c);
	}

	return normalized;
}

static String NormalizeElasticsearchFieldName(const String& fieldName)
{
	String normalized("");

	for (char& c : boost::trim_copy(fieldName)) {
	    // Elasticsearch uses dots for field separation, so we need to avoid them here.
		if (c == '.') {
			c = '_';
		}

		normalized += tolower(c);
	}

	return normalized;
}

EcsDocument::EcsDocument(String index, Dictionary::Ptr document)
	: m_Index(std::move(index)), m_Document(std::move(document))
{}
