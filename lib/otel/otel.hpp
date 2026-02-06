// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "base/io-engine.hpp"
#include "base/tlsstream.hpp"
#include "base/shared.hpp"
#include "base/shared-object.hpp"
#include "base/string.hpp"
#include "remote/httpmessage.hpp"
#include "otel/opentelemetry/proto/collector/metrics/v1/metrics_service.pb.h"
#include "otel/opentelemetry/proto/metrics/v1/metrics.pb.h"
#include "otel/opentelemetry/proto/resource/v1/resource.pb.h"
#include <boost/asio/steady_timer.hpp>
#include <google/protobuf/io/zero_copy_stream.h>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <optional>
#include <variant>

namespace icinga
{

/**
 * Connection parameters for connecting to an OpenTelemetry collector endpoint.
 *
 * @ingroup otel
 */
struct OTelConnInfo
{
	bool EnableTls{false};
	bool InsecureNoVerify{false};
	int Port;
	String Host;
	String TlsCaCrt;
	String TlsCrt;
	String TlsKey;
	String MetricsEndpoint;
	Dictionary::ConstPtr BasicAuth;
};

/**
 * Represents an OTel AnyValue[^1] used as attribute value in OTel attributes.
 *
 * OTel AnyValue can represent a wide range of data types, but for the purpose of this impl,
 * we only support the most common/scalar types: bool, int64_t, double, and String.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otel/common/#anyvalue
 */
using OTelAttrVal = std::variant<bool, int64_t, double, String>;

/**
 * Represents a collection of OTel attributes[^1] as key-value pairs.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otel/common/#attribute
 */
using OTelAttrsMap = std::map<String, OTelAttrVal>;

/**
 * Represents an OTel Gauge[^1] stream of data points.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otel/metrics/data-model/#gauge
 */
struct Gauge
{
	using Ptr = std::unique_ptr<Gauge>;

	/**
	 * Transforms the internal Gauge representation into an OTel Metric Protobuf object.
	 *
	 * This method transfers ownership of the internal Protobuf Gauge object to the provided Protobuf
	 * Metric object. Afterwards, the next @c Record call will create a new internal Protobuf Gauge
	 * object to hold new data points until the next @c Transform call.
	 *
	 * @param metric The OTel Metric Protobuf object to transform into.
	 */
	void Transform(opentelemetry::proto::metrics::v1::Metric* metric)
	{
		metric->set_allocated_gauge(ProtoGauge.release());
	}

	/**
	 * Checks if the Gauge has any recorded data points.
	 *
	 * @return true if the Gauge has no data points; false otherwise.
	 */
	bool IsEmpty() const
	{
		return ProtoGauge == nullptr || ProtoGauge->data_points_size() == 0;
	}

	/**
	 * Records a new data point in the Gauge.
	 *
	 * This method may modify the provided attributes vector by moving the key-value pairs
	 * into the underlying Protobuf data point representation. Thus, if you intend to reuse
	 * the attributes after calling this method, make sure to pass a copy of it.
	 *
	 * @param data The data point value to be recorded.
	 * @param attrs The attributes associated with the data point.
	 * @param start The start time of the data point in seconds.
	 * @param end The end time of the data point in seconds. Defaults to the current time.
	 *
	 * @tparam T The type of the data point value (int64_t or double).
	 *
	 * @return The byte size of the recorded data point.
	 */
	template <typename T>
	[[nodiscard]] std::size_t Record(T data, double start, double end, OTelAttrsMap&& attrs)
	{
		namespace ch = std::chrono;

		if (!ProtoGauge) {
			ProtoGauge.reset(opentelemetry::proto::metrics::v1::Gauge::default_instance().New());
		}

		auto* dataPoint = ProtoGauge->add_data_points();
		if constexpr (std::is_same_v<T, double>) {
			dataPoint->set_as_double(data);
		} else {
			static_assert(std::is_same_v<T, int64_t>, "Gauge can only be instantiated with int64_t or double types.");
			dataPoint->set_as_int(data);
		}

		dataPoint->set_start_time_unix_nano(
			static_cast<uint64_t>(ch::duration_cast<ch::nanoseconds>(ch::duration<double>(start)).count())
		);
		dataPoint->set_time_unix_nano(
			static_cast<uint64_t>(ch::duration_cast<ch::nanoseconds>(ch::duration<double>(end)).count())
		);

		for (auto& [key, value] : attrs) {
			if (key.IsEmpty()) {
				BOOST_THROW_EXCEPTION(std::invalid_argument("OTel attribute key cannot be empty."));
			}

			auto* attr = dataPoint->add_attributes();
			attr->set_key(std::move(key));
			std::visit([&attr](auto&& arg) {
				using ValT = std::decay_t<decltype(arg)>;
				auto* attrValue = attr->mutable_value();
				if constexpr (std::is_same_v<ValT, int64_t>) {
					attrValue->set_int_value(arg);
				} else if constexpr (std::is_same_v<ValT, double>) {
					attrValue->set_double_value(arg);
				} else if constexpr (std::is_same_v<ValT, bool>) {
					attrValue->set_bool_value(arg);
				} else if constexpr (std::is_same_v<ValT, String>) {
					attrValue->set_string_value(std::move(arg));
				} else {
					ABORT("Unsupported attribute value type in OTel Gauge data point.");
				}
			}, value);
		}
		return dataPoint->ByteSizeLong();
	}

private:
	std::unique_ptr<opentelemetry::proto::metrics::v1::Gauge> ProtoGauge{nullptr};
};

/**
 * OTel implements the OpenTelemetry Protocol (OTELP) exporter.
 *
 * This class manages the connection to an OpenTelemetry collector or compatible backend and
 * handles exporting (currently only metrics) in OTELP Protobuf format over HTTP. It supports
 * TLS connections, basic authentication, and implements retry logic for transient errors as
 * per OTel specs.
 *
 * @ingroup otel
 */
class OTel : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(OTel);

	// Protobuf request and response types for exporting metrics.
	using MetricsRequest = opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceRequest;
	using MetricsResponse = opentelemetry::proto::collector::metrics::v1::ExportMetricsServiceResponse;

	explicit OTel(OTelConnInfo& connInfo);

	void Start();
	void Stop();
	void Export(std::unique_ptr<MetricsRequest>& request);

	bool Exporting() const
	{
		std::lock_guard lock(m_Mutex);
		return m_Exporting;
	}

	bool Stopped() const { return m_Stopped.load(); }

	static void PopulateResourceAttrs(const std::unique_ptr<opentelemetry::proto::metrics::v1::ResourceMetrics>& rm, const String& instanceID);
	static void ValidateName(const std::string_view name);
	static bool IsRetryableExportError(boost::beast::http::status status);

private:
	OTel(OTelConnInfo& connInfo, boost::asio::io_context& io);

	void Connect(boost::asio::yield_context& yc);
	void ExportLoop(boost::asio::yield_context& yc);
	void Export(boost::asio::yield_context& yc) const;

	void ResetExporting(bool notifyAll = false);

	const OTelConnInfo m_ConnInfo;
	std::optional<AsioTlsOrTcpStream> m_Stream;
	Shared<boost::asio::ssl::context>::Ptr m_TlsContext;
	boost::asio::io_context::strand m_Strand;

	AsioDualEvent m_Export; // Event to signal when a new export request is available.
	boost::asio::steady_timer m_ConnTimer, m_RetryExportTimer;

	// Mutex and condition variable for synchronizing concurrent export requests.
	mutable std::mutex m_Mutex;
	std::condition_variable m_ExportCV;
	std::unique_ptr<MetricsRequest> m_Request; // Current export request being processed (if any).
	bool m_Exporting; // Whether an export operation is in progress.
	std::atomic_bool m_Stopped; // Whether someone has requested to stop the exporter.
};

/**
 * A zero-copy output stream that writes directly to an Asio [TLS] stream.
 *
 * This class implements the @c google::protobuf::io::ZeroCopyOutputStream interface, allowing Protobuf
 * serializers to write data directly to an Asio [TLS] stream without unnecessary copying of data. It
 * doesn't buffer data internally, but instead writes it in chunks to the underlying stream using an HTTP
 * request writer (@c HttpRequestWriter) in a Protobuf binary format. It is not safe to be reused across
 * multiple export calls.
 *
 * @ingroup otel
 */
class AsioProtobufOutStream final : public google::protobuf::io::ZeroCopyOutputStream
{
public:
	AsioProtobufOutStream(const AsioTlsOrTcpStream& stream, const OTelConnInfo& connInfo, boost::asio::yield_context& yc);

	bool Next(void** data, int* size) override;
	void BackUp(int count) override;
	int64_t ByteCount() const override;

	bool WriterDone();

private:
	void Flush(bool finish = false);

	int64_t m_Pos{0}; // Monotonically increasing byte position in the stream (excluding m_Buffered bytes)
	int64_t m_Buffered{0}; // Number of uncommitted bytes currently buffered.
	OutgoingHttpRequest m_Writer;
	boost::asio::yield_context& m_YieldContext; // Yield context for async operations.
};

/**
 * Backoff strategy for retrying failed OTel exports.
 *
 * This struct implements an exponential backoff strategy for retrying failed export attempts
 * to an OpenTelemetry collector. The backoff duration increases exponentially with each attempt,
 * up to a maximum limit. Currently, it doesn't apply any jitter to the backoff duration to keep
 * things simple but this could be added in the future if we intend to use concurrent exporters.
 *
 * @ingroup otel
 */
struct Backoff
{
	static constexpr std::chrono::milliseconds MaxBackoffMs = std::chrono::seconds(30);
	static constexpr std::chrono::milliseconds MinBackoffMs = std::chrono::milliseconds(100);

	std::chrono::milliseconds operator()(uint64_t attempt) const
	{
		using namespace std::chrono;

		// 2^attempt may overflow, so we cap it to a safe value within the 64-bit range,
		// which is sufficient to reach MaxBackoffMs from MinBackoffMs.
		constexpr uint64_t maxSafeAttempt = 16; // 2^16 * 100ms = 6553.6s > 30s
		auto exponential = MinBackoffMs * (1ULL << std::min(attempt, maxSafeAttempt));
		if (exponential >= MaxBackoffMs) {
			return MaxBackoffMs;
		}
		return duration_cast<milliseconds>(exponential);
	}
};

/**
 * Exception class representing a retryable export error.
 *
 * This exception is thrown when an export attempt to an OpenTelemetry collector fails
 * with a retryable error status. It carries an optional HTTP throttle[^1] duration indicating
 * how long to wait before retrying the export.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otlp/#otlphttp-throttling
 *
 * @ingroup otel
 */
struct RetryableExportError : std::exception
{
	explicit RetryableExportError(uint64_t throttle): m_Throttle(throttle)
	{
	}

	uint64_t Throttle() const { return m_Throttle; }
	const char* what() const noexcept override
	{
		return "OTel::RetryableExportError()";
	}

private:
	uint64_t m_Throttle;
};

} // namespace icinga
