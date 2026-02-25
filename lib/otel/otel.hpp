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
	bool SecureVerify{true};
	int Port;
	String Host;
	String TlsCaCrt;
	String TlsCrt;
	String TlsKey;
	String MetricsEndpoint;
	String BasicAuth; // Base64-encoded "username:password" string for basic authentication.
};

/**
 * OTel implements the OpenTelemetry Protocol (OTLP) exporter.
 *
 * This class manages the connection to an OpenTelemetry collector or compatible backend and
 * handles exporting (currently only metrics) in OTLP Protobuf format over HTTP. It supports
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
	// Protobuf attribute type used for OTel resource and data point attributes.
	using Attribute = opentelemetry::proto::common::v1::KeyValue;
	// Protobuf Gauge type used for representing OTel Gauge metric streams.
	using Gauge = opentelemetry::proto::metrics::v1::Gauge;

	/**
	 * Represents a collection of OTel attributes[^1] as key-value pairs.
	 *
	 * [^1]: https://opentelemetry.io/docs/specs/otel/common/#attribute
	 */
	using AttrsMap = std::map<String, Value>;

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
	template<typename Key, typename AttrVal>
	static void SetAttribute(Attribute& attr, Key&& key, AttrVal&& value);
	static bool IsRetryableExportError(boost::beast::http::status status);

	template <typename T>
	[[nodiscard]] static std::size_t Record(Gauge& gauge, T data, double start, double end, AttrsMap& attrs);

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
	// Timer for scheduling retries of failed exports and reconnection attempts.
	boost::asio::steady_timer m_RetryExportAndConnTimer;

	// Mutex and condition variable for synchronizing concurrent export requests.
	mutable std::mutex m_Mutex;
	std::condition_variable m_ExportCV;
	std::unique_ptr<MetricsRequest> m_Request; // Current export request being processed (if any).
	bool m_Exporting; // Whether an export operation is in progress.
	std::atomic_bool m_Stopped; // Whether someone has requested to stop the exporter.
};
extern template std::size_t OTel::Record(Gauge&, int64_t, double, double, AttrsMap&);
extern template std::size_t OTel::Record(Gauge&, double, double, double, AttrsMap&);
extern template void OTel::SetAttribute(Attribute&, std::string_view&&, String&&);
extern template void OTel::SetAttribute(Attribute&, String&&, Value&);

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
	AsioProtobufOutStream(const AsioTlsOrTcpStream& stream, const OTelConnInfo& connInfo, boost::asio::yield_context yc);

	bool Next(void** data, int* size) override;
	void BackUp(int count) override;
	int64_t ByteCount() const override;

	bool WriterDone();

private:
	void Flush(bool finish = false);

	int64_t m_Pos{0}; // Monotonically increasing byte position in the stream (excluding m_Buffered bytes).
	std::size_t m_Buffered{0}; // Number of uncommitted bytes currently buffered.
	OutgoingHttpRequest m_Writer;
	boost::asio::yield_context m_YieldContext; // Yield context for async operations.
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
	explicit RetryableExportError(uint64_t throttle): m_Throttle{std::chrono::seconds(throttle)}
	{
	}

	std::chrono::seconds Throttle() const { return m_Throttle; }
	const char* what() const noexcept override
	{
		return "OTel::RetryableExportError()";
	}

private:
	std::chrono::seconds m_Throttle;
};

} // namespace icinga
