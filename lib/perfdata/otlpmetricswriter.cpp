// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "perfdata/otlpmetricswriter.hpp"
#include "perfdata/otlpmetricswriter-ti.cpp"
#include "base/base64.hpp"
#include "base/defer.hpp"
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
static constexpr std::string_view l_PerfdataMetric = "state_check.perfdata";
static constexpr std::string_view l_ThresholdMetric = "state_check.threshold";

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
	connInfo.SecureVerify = !GetTlsInsecureNoverify();
	connInfo.Host = GetHost();
	connInfo.Port = GetPort();
	connInfo.TlsCaCrt = GetTlsCaFile();
	connInfo.TlsCrt = GetTlsCertFile();
	connInfo.TlsKey = GetTlsKeyFile();
	connInfo.MetricsEndpoint = GetMetricsEndpoint();
	if (auto auth = GetBasicAuth(); auth) {
		connInfo.BasicAuth = Base64::Encode(auth->Get("username") + ":" + auth->Get("password"));
	}

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
		if (m_TimerFlushInProgress.exchange(true, std::memory_order_relaxed)) {
			// Previous timer-initiated flush still in progress, skip this one.
			return;
		}
		m_WorkQueue.Enqueue([this] {
			Defer resetTimerFlag{[this] { m_TimerFlushInProgress.store(false, std::memory_order_relaxed); }};
			Flush(true);
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

			OTel::AttrsMap attrs{{"perfdata_label", pdv->GetLabel()}};
			if (auto unit = pdv->GetUnit(); !unit.IsEmpty()) {
				attrs.emplace("unit", std::move(unit));
			}
			AddBytesAndFlushIfNeeded(Record(checkable, l_PerfdataMetric, pdv->GetValue(), startTime, endTime, attrs));

			if (GetEnableSendThresholds()) {
				std::vector<std::pair<String, Value>> thresholds {
					{"critical", pdv->GetCrit()},
					{"warning", pdv->GetWarn()},
					{"min", pdv->GetMin()},
					{"max", pdv->GetMax()},
				};
				for (auto& [label, threshold] : thresholds) {
					if (!threshold.IsEmpty()) {
						attrs = {
							{"perfdata_label", pdv->GetLabel()},
							{"threshold_type", std::move(label)},
						};
						AddBytesAndFlushIfNeeded(
							Record(
								checkable,
								l_ThresholdMetric,
								Convert::ToDouble(threshold),
								startTime,
								endTime,
								attrs
							)
						);
					}
				}
			}
		}
	});
}

void OTLPMetricsWriter::Flush(bool fromTimer)
{
	// If previous export is still in progress and this flush is requested from timer, skip it.
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
		request->mutable_resource_metrics()->AddAllocated(metrics.ResourceMetrics.release());
	}
	if (request->resource_metrics_size() == 0) {
		Log(LogDebug, "OTLPMetricsWriter")
			<< "Not flushing OTel metrics: No data points recorded.";
		return;
	}
	m_Exporter->Export(request);
	m_RecordedBytes.store(0, std::memory_order_relaxed);
	m_DataPointsCount.store(0, std::memory_order_relaxed);
}

void OTLPMetricsWriter::AddBytesAndFlushIfNeeded(std::size_t newBytes)
{
	auto existingBytes = m_RecordedBytes.fetch_add(newBytes, std::memory_order_relaxed);
	if (auto bytes{existingBytes + newBytes}; bytes >= static_cast<uint64_t>(GetFlushThreshold())) {
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
 *
 * @param checkable The configuration object to associate the metric with.
 * @param metric The OTel metric enum value indicating which metric stream to record the data point for.
 * @param value The data point value to record.
 * @param startTime The start time of the data point in seconds.
 * @param endTime The end time of the data point in seconds.
 * @param attrs The attributes associated with the data point.
 *
 * @return The number of bytes recorded for this data point, which contributes to the flush threshold.
 */
template<typename T>
std::size_t OTLPMetricsWriter::Record(
	const Checkable::Ptr& checkable,
	std::string_view metric,
	T value,
	double startTime,
	double endTime,
	OTel::AttrsMap& attrs
)
{
	std::size_t bytes = 0;
	auto& metricsForObj = m_Metrics[checkable];
	if (metricsForObj.ResourceMetrics == nullptr) {
		using namespace std::string_view_literals;

		if (metricsForObj.ServiceInstanceId.IsEmpty()) {
			// Use instance ID composed of checkable name and service namespace to ensure uniqueness as per OTel specs.
			// See https://opentelemetry.io/docs/specs/semconv/resource/service/#service-instance.
			Array::Ptr data = new Array{{checkable->GetName(), GetServiceNamespace()}};
			metricsForObj.ServiceInstanceId = SHA1(PackObject(data));
		}
		metricsForObj.ResourceMetrics = std::make_unique<opentelemetry::proto::metrics::v1::ResourceMetrics>();
		metricsForObj.ResourceMetrics->add_scope_metrics(); // Pre-create ScopeMetrics entry.
		OTel::PopulateResourceAttrs(metricsForObj.ResourceMetrics, metricsForObj.ServiceInstanceId);

		auto* resource = metricsForObj.ResourceMetrics->mutable_resource();
		auto* attr = resource->add_attributes();
		OTel::SetAttribute(*attr, "service.namespace"sv, GetServiceNamespace());

		auto [host, service] = GetHostService(checkable);
		attr = resource->add_attributes();
		OTel::SetAttribute(*attr, "icinga2.host.name"sv, host->GetName());

		// Add entity reference (https://opentelemetry.io/docs/specs/otel/entities/data-model/).
		auto* entity = resource->add_entity_refs();
		entity->mutable_id_keys()->Add("icinga2.host.name");
		if (service) {
			entity->set_type("service");
			entity->mutable_id_keys()->Add("icinga2.service.name");

			attr = resource->add_attributes();
			OTel::SetAttribute(*attr, "icinga2.service.name"sv, service->GetShortName());
		} else {
			entity->set_type("host");
		}
		attr = resource->add_attributes();
		OTel::SetAttribute(*attr, "icinga2.command.name"sv, checkable->GetCheckCommand()->GetName());
		bytes = metricsForObj.ResourceMetrics->ByteSizeLong();
	}

	auto* sm = metricsForObj.ResourceMetrics->mutable_scope_metrics(0);
	auto* metrics = sm->mutable_metrics();
	auto it = std::find_if(metrics->begin(), metrics->end(), [metric](const auto& m) { return m.name() == metric; });
	OTel::Gauge* gaugePtr = nullptr;
	if (it == metrics->end()) {
		OTel::ValidateName(metric);
		auto* metricPtr = sm->add_metrics();
		metricPtr->set_name(std::string(metric));
		bytes += metricPtr->ByteSizeLong(); // Account for metric name size in bytes.
		gaugePtr = metricPtr->mutable_gauge();
	} else {
		gaugePtr = it->mutable_gauge();
	}
	bytes += OTel::Record(*gaugePtr, value, startTime, endTime, attrs);
	m_DataPointsCount.fetch_add(1, std::memory_order_relaxed);
	return bytes;
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
