/* Icinga 2 | (c) 2026 Icinga GmbH | GPLv2+ */

#include "config.h"
#ifdef ICINGA2_WITH_OPENTELEMETRY
#include "perfdata/otlpmetricswriter.hpp"
#include "perfdata/otlpmetricswriter-ti.cpp"
#include "base/json.hpp"
#include "base/object-packer.hpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"
#include "icinga/checkable.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/service.hpp"
#include <future>

using namespace icinga;

REGISTER_TYPE(OTLPMetricsWriter);

REGISTER_STATSFUNCTION(OTLPMetricsWriter, &OTLPMetricsWriter::StatsFunc);

// Represent our currently supported metric streams.
//
// Note: These and all other attribute keys used within this compilation unit follow
// the OTel general naming guidelines[^1] and conventions[^2].
//
// [^1]: https://opentelemetry.io/docs/specs/semconv/general/metrics/#general-guidelines
// [^2]: https://opentelemetry.io/docs/specs/semconv/general/naming
static const String l_PerfdataMetricName = "state_check.perfdata";
static const String l_ThresholdCritMetric = "state_check.threshold.critical";
static const String l_ThresholdWarnMetric = "state_check.threshold.warning";
static const String l_ThresholdMinMetric = "state_check.threshold.min";
static const String l_ThresholdMaxMetric = "state_check.threshold.max";

void OTLPMetricsWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData statusData;
	for (const Ptr& otlpWriter : ConfigType::GetObjectsByType<OTLPMetricsWriter>()) {
		std::size_t workQueueSize = otlpWriter->m_WorkQueue.GetLength();
		double workQueueItemRate = otlpWriter->m_WorkQueue.GetTaskCount(60) / 60.0;
		std::size_t dataPointsCount = otlpWriter->m_DataPointsCount.load(std::memory_order_relaxed);
		uint64_t messageSize = otlpWriter->m_RecordedBytes.load(std::memory_order_relaxed);

		const auto name = otlpWriter->GetName();
		statusData.emplace_back(name, new Dictionary{
			{"work_queue_items", workQueueSize},
			{"work_queue_item_rate", workQueueItemRate},
			{"data_buffer_items", dataPointsCount},
			{"data_buffer_bytes", messageSize},
		});

		perfdata->Add(new PerfdataValue("otlpmetricswriter_" + name + "_work_queue_items", workQueueSize, true));
		perfdata->Add(new PerfdataValue("otlpmetricswriter_" + name + "_work_queue_item_rate", workQueueItemRate));
		perfdata->Add(new PerfdataValue("otlpmetricswriter_" + name + "_data_buffer_items", dataPointsCount, true));
		perfdata->Add(new PerfdataValue("otlpmetricswriter_" + name + "_data_buffer_bytes", messageSize, false, "bytes"));
	}
	status->Set("otlpmetricswriter", new Dictionary{std::move(statusData)});
}

void OTLPMetricsWriter::OnConfigLoaded()
{
	ObjectImpl::OnConfigLoaded();

	m_WorkQueue.SetName("OTLPMetricsWriter, " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, "OTLPMetricsWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

void OTLPMetricsWriter::OnAllConfigLoaded()
{
	ObjectImpl::OnAllConfigLoaded();

	OTelConnInfo connInfo;
	connInfo.EnableTls = GetEnableTls();
	connInfo.InsecureNoVerify = GetTlsInsecureNoverify();
	connInfo.Host = GetHost();
	connInfo.Port = GetPort();
	connInfo.TlsCaCrt = GetTlsCaFile();
	connInfo.TlsCrt = GetTlsCertFile();
	connInfo.TlsKey = GetTlsKeyFile();
	connInfo.MetricsEndpoint = GetMetricsEndpoint();
	connInfo.BasicAuth = GetBasicAuth();

	m_Exporter.reset(new OTel{connInfo});
}

void OTLPMetricsWriter::Resume()
{
	ObjectImpl::Resume();

	Log(LogInformation, "OTLPMetricsWriter")
		<< "'" << GetName() << "' resumed.";

	m_WorkQueue.SetExceptionCallback([](boost::exception_ptr exp) {
		Log(LogCritical, "OTLPMetricsWriter")
			<< "Exception while producing OTel metric: " << DiagnosticInformation(exp);
	});

	m_FlushTimer = Timer::Create();
	m_FlushTimer->SetInterval(GetFlushInterval());
	m_FlushTimer->OnTimerExpired.connect([this](const Timer* const&) {
		if (m_TimerFlushInProgress.exchange(true)) {
			// Previous timer-initiated flush still in progress, skip this one.
			return;
		}
		m_WorkQueue.Enqueue([this] {
			Flush(true);
			m_TimerFlushInProgress.store(false);
		});
	});
	m_FlushTimer->Start();
	m_Exporter->Start();

	m_CheckResultsSlot = Checkable::OnNewCheckResult.connect([this](
		const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr,
		const MessageOrigin::Ptr&
	) {
		CheckResultHandler(checkable, cr);
	});
	m_ActiveChangedSlot = OnActiveChanged.connect([this](const ConfigObject::Ptr& obj, const Value&) {
		auto checkable = dynamic_pointer_cast<Checkable>(obj);
		if (!checkable || checkable->IsActive()) {
			return;
		}
		m_WorkQueue.Enqueue([this, checkable] { m_Metrics.erase(checkable); });
	});
}

void OTLPMetricsWriter::Pause()
{
	m_CheckResultsSlot.disconnect();
	m_ActiveChangedSlot.disconnect();

	m_FlushTimer->Stop(true);

	std::promise<void> promise;
	auto future = promise.get_future();
	m_WorkQueue.Enqueue([this, &promise] {
		Flush();
		promise.set_value();
	}, PriorityLow);

	if (auto status = future.wait_for(std::chrono::seconds(GetDisconnectTimeout())); status != std::future_status::ready) {
		Log(LogWarning, "OTLPMetricsWriter")
			<< "Disconnect timeout reached while flushing OTel metrics, discarding '" << m_DataPointsCount
			<< "' data points ('" << m_RecordedBytes << "' bytes).";
	}
	m_Exporter->Stop();
	m_WorkQueue.Join();

	m_Metrics.clear();

	Log(LogInformation, "OTLPMetricsWriter")
		<< "'" << GetName() << "' paused.";

	ObjectImpl::Pause();
}

void OTLPMetricsWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata() || !cr->GetPerformanceData()) {
		return;
	}

	m_WorkQueue.Enqueue([this, checkable, cr] {
		if (m_Exporter->Stopped()) {
			return;
		}
		CONTEXT("Processing check result for '" << checkable->GetName() << "'.");

		auto startTime = cr->GetScheduleStart();
		auto endTime = cr->GetExecutionEnd();

		Array::Ptr perfdata = cr->GetPerformanceData();
		ObjectLock olock(perfdata);
		for (const Value& val : perfdata) {
			PerfdataValue::Ptr pdv;
			if (val.IsObjectType<PerfdataValue>()) {
				pdv = val;
			} else {
				try {
					pdv = PerfdataValue::Parse(val);
				} catch (const std::exception&) {
					Log(LogWarning, "OTLPMetricsWriter")
						<< "Ignoring invalid perfdata for checkable '" << checkable->GetName() << "' and command '"
						<< checkable->GetCheckCommand()->GetName() << "' with value: " << val;
					continue;
				}
			}

			OTelAttrsMap attrs{{"label", pdv->GetLabel()}};
			if (auto unit = pdv->GetUnit(); !unit.IsEmpty()) {
				attrs.emplace("unit", std::move(unit));
			}
			Record(checkable, l_PerfdataMetricName, pdv->GetValue(), startTime, endTime, std::move(attrs));

			if (GetEnableSendThresholds()) {
				if (auto crit = pdv->GetCrit(); !crit.IsEmpty()) {
					Record(checkable, l_ThresholdCritMetric, Convert::ToDouble(crit), startTime, endTime, {});
				}
				if (auto warn = pdv->GetWarn(); !warn.IsEmpty()) {
					Record(checkable, l_ThresholdWarnMetric, Convert::ToDouble(warn), startTime, endTime, {});
				}
				if (auto min = pdv->GetMin(); !min.IsEmpty()) {
					Record(checkable, l_ThresholdMinMetric, Convert::ToDouble(min), startTime, endTime, {});
				}
				if (auto max = pdv->GetMax(); !max.IsEmpty()) {
					Record(checkable, l_ThresholdMaxMetric, Convert::ToDouble(max), startTime, endTime, {});
				}
			}
		}
	});
}

void OTLPMetricsWriter::Flush(bool fromTimer)
{
	// If previous export still in progress and this flush is requested from timer, skip it.
	// For manual flushes (e.g., due to reaching flush threshold), we want to block until
	// the previous export is done before returning to the caller (blocking is handled in OTel::Export()).
	if (fromTimer && m_Exporter->Exporting()) {
		return;
	}

	Log(LogDebug, "OTLPMetricsWriter")
		<< "Flushing OTel metrics to OpenTelemetry collector" << (fromTimer ? " (timer expired)." : ".");

	auto request = std::make_unique<OTel::MetricsRequest>();
	for (auto& [checkable, metrics] : m_Metrics) {
		if (metrics.ResourceMetrics == nullptr) {
			continue; // No metrics recorded for this checkable (see Record()).
		}
		auto sm = metrics.ResourceMetrics->mutable_scope_metrics(0);
		for (const auto& [metricName, gauge] : metrics.Metrics) {
			if (gauge->IsEmpty()) {
				continue;
			}
			auto metric = sm->add_metrics();
			metric->set_name(metricName);
			gauge->Transform(metric);
		}
		if (sm->metrics_size() > 0) {
			request->mutable_resource_metrics()->AddAllocated(metrics.ResourceMetrics.release());
		}
	}
	if (request->resource_metrics_size() == 0) {
		Log(LogDebug, "OTLPMetricsWriter")
			<< "Not flushing OTel metrics: No data points recorded.";
		return; // Nothing to export.
	}
	m_Exporter->Export(request);
	m_RecordedBytes.store(0, std::memory_order_relaxed);
	m_DataPointsCount.store(0, std::memory_order_relaxed);
}

void OTLPMetricsWriter::FlushIfNeeded()
{
	if (auto bytes = m_RecordedBytes.load(std::memory_order_relaxed); bytes >= static_cast<uint64_t>(GetFlushThreshold())) {
		Log(LogDebug, "OTLPMetricsWriter")
			<< "Flush threshold reached, flushing '" << bytes << "' bytes of OTel metrics.";
		Flush();
	}
}

/**
 * Record a data point for the specified OTel metric associated with the given configuration object.
 *
 * This method records a data point of type T for the specified metric name associated with the
 * provided configuration object. If the metric does not exist for the object, it is created.
 *
 * @tparam T The type of the data point to record (e.g., int64_t, double).
 * @param checkable The configuration object to associate the metric with.
 * @param metric The name of the metric to record the data point for.
 * @param value The data point value to record.
 * @param startTime The start time of the data point in seconds.
 * @param endTime The end time of the data point in seconds.
 * @param attrs The attributes associated with the data point.
 */
template<typename T>
void OTLPMetricsWriter::Record(
	const Checkable::Ptr& checkable,
	const String& metric,
	T value,
	double startTime,
	double endTime,
	OTelAttrsMap&& attrs
) {
	auto& metricsForObj = m_Metrics[checkable];
	if (metricsForObj.ResourceMetrics == nullptr) {
		if (metricsForObj.ServiceInstanceId.IsEmpty()) {
			// Use instance ID composed of checkable name and service namespace to ensure uniqueness as per OTel specs.
			// See https://opentelemetry.io/docs/specs/semconv/resource/service/#service-instance.
			Array::Ptr data = new Array{{checkable->GetName(), GetServiceNamespace()}};
			metricsForObj.ServiceInstanceId = SHA1(PackObject(data));
		}
		metricsForObj.ResourceMetrics = std::make_unique<opentelemetry::proto::metrics::v1::ResourceMetrics>();
		metricsForObj.ResourceMetrics->add_scope_metrics(); // Pre-create ScopeMetrics entry.
		OTel::PopulateResourceAttrs(metricsForObj.ResourceMetrics, metricsForObj.ServiceInstanceId);

		auto resource = metricsForObj.ResourceMetrics->mutable_resource();
		auto attr = resource->add_attributes();
		attr->set_key("service.namespace");
		attr->mutable_value()->set_string_value(GetServiceNamespace());

		auto [host, service] = GetHostService(checkable);
		attr = resource->add_attributes();
		attr->set_key("icinga2.host.name");
		attr->mutable_value()->set_string_value(host->GetName());

		// Add entity reference (https://opentelemetry.io/docs/specs/otel/entities/data-model/).
		auto entity = resource->add_entity_refs();
		entity->mutable_id_keys()->Add("icinga2.host.name");
		if (service) {
			entity->set_type("service");
			entity->mutable_id_keys()->Add("icinga2.service.name");

			attr = resource->add_attributes();
			attr->set_key("icinga2.service.name");
			attr->mutable_value()->set_string_value(service->GetShortName());
		} else {
			entity->set_type("host");
		}
		attr = resource->add_attributes();
		attr->set_key("icinga2.command.name");
		attr->mutable_value()->set_string_value(checkable->GetCheckCommand()->GetName());

		m_RecordedBytes.fetch_add(metricsForObj.ResourceMetrics->ByteSizeLong(), std::memory_order_relaxed);
	}

	auto it = metricsForObj.Metrics.find(metric);
	if (it == metricsForObj.Metrics.end()) {
		OTel::ValidateName(metric.GetData());
		auto pair = metricsForObj.Metrics.emplace(metric, std::make_unique<Gauge>());
		it = pair.first;
	}
	m_RecordedBytes.fetch_add(it->second->Record(value, startTime, endTime, std::move(attrs)), std::memory_order_relaxed);
	m_DataPointsCount.fetch_add(1, std::memory_order_relaxed);
	FlushIfNeeded();
}

void OTLPMetricsWriter::ValidatePort(const Lazy<int>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl::ValidatePort(lvalue, utils);
	if (auto p = lvalue(); p < 1 || p > 65535) {
		BOOST_THROW_EXCEPTION(ValidationError(this, {"port"}, "Port must be in the range 1-65535."));
	}
}

void OTLPMetricsWriter::ValidateFlushInterval(const Lazy<int>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl::ValidateFlushInterval(lvalue, utils);
	if (lvalue() < 1) {
		BOOST_THROW_EXCEPTION(ValidationError(this, {"flush_interval"}, "Flush interval must be at least 1 second."));
	}
}

void OTLPMetricsWriter::ValidateFlushThreshold(const Lazy<int64_t>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl::ValidateFlushThreshold(lvalue, utils);
	if (lvalue() < 1) {
		BOOST_THROW_EXCEPTION(ValidationError(this, {"flush_threshold"}, "Flush threshold must be at least 1."));
	}
	// Protobuf limits the size of messages to be serialiazed/deserialized to max 2GiB. Thus, we can't accept
	// a flush threshold that would exceed that limit with a reasonable safe margin of 10MiB for any other
	// overhead in the message not accounted for in @c m_RecordedBytes.
	// See https://protobuf.dev/programming-guides/proto-limits/#total.
	constexpr std::size_t maxMessageSize = 2ULL * 1024 * 1024 * 1024 - 10 * 1024 * 1024;
	if (static_cast<uint64_t>(lvalue()) > maxMessageSize) {
		BOOST_THROW_EXCEPTION(ValidationError(
			this,
			{"flush_threshold"},
			"Flush threshold too high, would exceed Protobuf message size limit of 2GiB (1.9GiB max allowed)."
		));
	}
}

#endif // ICINGA2_WITH_OPENTELEMETRY
