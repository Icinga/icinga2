// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "otel/otel.hpp"
#include "base/application.hpp"
#include "base/base64.hpp"
#include "base/defer.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsutility.hpp"
#include <boost/asio/read.hpp>
#include <boost/beast/http/message.hpp>
#include <future>

using namespace icinga;

namespace http = boost::beast::http;
namespace v1_metrics = opentelemetry::proto::metrics::v1;

// The max buffer size used to batch Protobuf writes to Asio streams.
static constexpr std::size_t l_BufferSize = 64UL * 1024;
// The OpenTelemetry schema convention URL used in the exported metrics.
// See https://opentelemetry.io/docs/specs/semconv/
static constexpr auto l_OTelSchemaConv = "https://opentelemetry.io/schemas/1.39.0";

template std::size_t OTel::Record(Gauge&, int64_t, double, double, AttrsMap&&);
template std::size_t OTel::Record(Gauge&, double, double, double, AttrsMap&&);

OTel::OTel(OTelConnInfo& connInfo): OTel{connInfo, IoEngine::Get().GetIoContext()}
{
}

OTel::OTel(OTelConnInfo& connInfo, boost::asio::io_context& io)
	: m_ConnInfo{std::move(connInfo)},
	  m_Strand{io},
	  m_Export{io},
	  m_RetryExportAndConnTimer{io},
	  m_Exporting{false},
	  m_Stopped{false}
{
	if (m_ConnInfo.EnableTls) {
		m_TlsContext = MakeAsioSslContext(m_ConnInfo.TlsCrt, m_ConnInfo.TlsKey, m_ConnInfo.TlsCaCrt);
	}
}

void OTel::Start()
{
	if (m_Stopped.exchange(false)) {
		ResetExporting(true);
	}

	ConstPtr keepAlive(this);
	IoEngine::SpawnCoroutine(m_Strand, [this, keepAlive](boost::asio::yield_context yc) { ExportLoop(yc); });
}

/**
 * Stop the OTel exporter and disconnect from the OpenTelemetry collector.
 *
 * This method blocks until the exporter has fully stopped and disconnected from the collector.
 * It cancels any ongoing export operations and clears all its internal state, so that it can be
 * safely restarted later if needed.
 */
void OTel::Stop()
{
	if (m_Stopped.exchange(true)) {
		return;
	}

	std::promise<void> promise;
	IoEngine::SpawnCoroutine(m_Strand, [this, &promise, keepAlive = ConstPtr(this)](boost::asio::yield_context& yc) {
		m_Export.Set();
		m_RetryExportAndConnTimer.cancel();

		if (!m_Stream) {
			promise.set_value();
			return;
		}

		std::visit([this, &yc](auto& stream) {
			{
				Timeout writerTimeout(m_Strand, boost::posix_time::seconds(5), [&stream] {
					boost::system::error_code ec;
					stream->lowest_layer().cancel(ec);
				});
				m_Export.WaitForClear(yc);
			}

			using StreamType = std::decay_t<decltype(stream)>;
			if constexpr (std::is_same_v<StreamType, Shared<AsioTlsStream>::Ptr>) {
				stream->GracefulDisconnect(m_Strand, yc);
			} else {
				static_assert(std::is_same_v<StreamType, Shared<AsioTcpStream>::Ptr>, "Unknown stream type");
				boost::system::error_code ec;
				stream->lowest_layer().shutdown(AsioTcpStream::lowest_layer_type::shutdown_both, ec);
				stream->lowest_layer().close(ec);
			}
		}, *m_Stream);

		Log(LogInformation, "OTelExporter")
			<< "Disconnected from OpenTelemetry collector.";

		m_Stream.reset();
		m_Request.reset();
		promise.set_value();
	});
	promise.get_future().wait();
}

/**
 * Export the given OTel metrics request to the OpenTelemetry collector.
 *
 * This method initiates the export of the provided OTel metrics request to the configured
 * OpenTelemetry collector. If an export is already in progress, it waits for the previous
 * export to complete before proceeding with the new export request (blocking the caller).
 *
 * @param request The OTel metrics request to export.
 */
void OTel::Export(std::unique_ptr<MetricsRequest>& request)
{
	std::unique_lock lock(m_Mutex);
	if (m_Exporting) {
		Log(LogWarning, "OTelExporter")
			<< "Received export request while previous export is still in progress. Waiting for it to complete.";

		m_ExportCV.wait(lock, [this] { return m_Stopped || !m_Exporting; });
		if (m_Stopped) {
			return;
		}
	}
	m_Exporting = true;
	lock.unlock();

	// Access to m_Request is serialized via m_Strand, so we must post the actual export operation to it.
	boost::asio::post(m_Strand, [this, keepAlive = ConstPtr(this), request = std::move(request)]() mutable {
		m_Request = std::move(request);
		m_Export.Set();
	});
}

/**
 * Populate the standard OTel resource attributes in the given ResourceMetrics Protobuf object.
 *
 * This method populates the standard OTel resource attributes as per OTel specifications[^1][^2]
 * into the provided ResourceMetrics Protobuf object. It sets attributes such as service name,
 * instance ID, version, and telemetry SDK information.
 *
 * @param rm The ResourceMetrics Protobuf object to populate.
 * @param instanceID The unique instance ID for the service instance to set in the resource attributes.
 *
 * [^1]: https://opentelemetry.io/docs/specs/semconv/resource/#telemetry-sdk
 * [^2]: https://opentelemetry.io/docs/specs/semconv/resource/service/
 */
void OTel::PopulateResourceAttrs(const std::unique_ptr<v1_metrics::ResourceMetrics>& rm, const String& instanceID)
{
	rm->set_schema_url(l_OTelSchemaConv);
	auto* resource = rm->mutable_resource();

	auto attr = resource->add_attributes();
	attr->set_key("service.name");
	attr->mutable_value()->set_string_value("Icinga 2");

	attr = resource->add_attributes();
	attr->set_key("service.instance.id");
	attr->mutable_value()->set_string_value(instanceID);

	attr = resource->add_attributes();
	attr->set_key("service.version");
	attr->mutable_value()->set_string_value(Application::GetAppVersion());

	attr = resource->add_attributes();
	// We don't actually use OTel SDKs here, but to comply with OTel specs, we need to provide these attributes anyway.
	attr->set_key("telemetry.sdk.language");
	attr->mutable_value()->set_string_value("cpp");

	attr = resource->add_attributes();
	attr->set_key("telemetry.sdk.name");
	attr->mutable_value()->set_string_value("Icinga 2 OTel Integration");

	attr = resource->add_attributes();
	attr->set_key("telemetry.sdk.version");
	attr->mutable_value()->set_string_value(Application::GetAppVersion());

	// There is only a single instrumentation scope for Icinga 2 metrics and assume it's pre-created by the caller.
	auto ism = rm->mutable_scope_metrics(0);
	ism->set_schema_url(l_OTelSchemaConv);
	ism->mutable_scope()->set_name("icinga2");
	ism->mutable_scope()->set_version(Application::GetAppVersion());
}

/**
 * Establish a connection to the OpenTelemetry collector endpoint.
 *
 * In case of connection failures, it retries as per OTel spec[^1] with exponential backoff until a successful
 * connection is established or the exporter is stopped. Therefore, @c m_Stream is not guaranteed to be valid
 * after this method returns, so the caller must check it before using it.
 *
 * @param yc The Boost.Asio yield context for asynchronous operations.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otlp/#otlphttp-connection
 */
void OTel::Connect(boost::asio::yield_context& yc)
{
	while (!m_Stopped) {
		Log(LogInformation, "OTelExporter")
			<< "Connecting to OpenTelemetry collector on host '" << m_ConnInfo.Host << ":" << m_ConnInfo.Port << "'.";

		for (uint64_t attempt = 1; true; ++attempt) {
			try {
				decltype(m_Stream) stream;
				if (m_ConnInfo.EnableTls) {
					stream = Shared<AsioTlsStream>::Make(m_Strand.context(), *m_TlsContext, m_ConnInfo.Host);
				} else {
					stream = Shared<AsioTcpStream>::Make(m_Strand.context());
				}

				std::visit(
					[this, &yc](auto& stream) {
						icinga::Connect(stream->lowest_layer(), m_ConnInfo.Host, std::to_string(m_ConnInfo.Port), yc);
						if constexpr (std::is_same_v<std::decay_t<decltype(stream)>, Shared<AsioTlsStream>::Ptr>) {
							auto& tlsStream = stream->next_layer();
							tlsStream.async_handshake(tlsStream.client, yc);

							if (!m_ConnInfo.InsecureNoVerify) {
								if (!tlsStream.GetPeerCertificate()) {
									BOOST_THROW_EXCEPTION(std::runtime_error(
										"TLS certificate validation failed: OpenTelemetry collector didn't present any TLS certificate."
									));
								}
								if (!tlsStream.IsVerifyOK()) {
									BOOST_THROW_EXCEPTION(std::runtime_error(
										"TLS certificate validation failed: " + std::string(tlsStream.GetVerifyError())
									));
								}
							}
						}
					},
					*stream
				);
				m_Stream = std::move(stream);

				Log(LogInformation, "OTelExporter")
					<< "Successfully connected to OpenTelemetry collector.";
				return;
			} catch (const std::exception& ex) {
				Log(m_Stopped ? LogDebug : LogCritical, "OTelExporter")
					<< "Cannot connect to OpenTelemetry collector '" << m_ConnInfo.Host << ":" << m_ConnInfo.Port
					<< "' (attempt #" << attempt << "): " << ex.what();

				if (m_Stopped) {
					return;
				}
			}

			boost::system::error_code ec;
			m_RetryExportAndConnTimer.expires_after(Backoff{}(attempt));
			m_RetryExportAndConnTimer.async_wait(yc[ec]);

			if (m_Stopped) {
				return;
			}
		}
	}
}

/**
 * Main export loop for exporting OTel metrics to the configured collector.
 *
 * This method runs in a loop, waiting for new metrics to be available for export. In case of export failures,
 * it retries the export as per OTel spec[^1] with exponential backoff until the export succeeds or the exporter
 * is stopped. After a successful export, it clears the exported metrics from @c m_Request to make room for new metrics.
 *
 * @param yc The Asio yield context for asynchronous operations.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otlp/#retryable-response-codes
 */
void OTel::ExportLoop(boost::asio::yield_context& yc)
{
	Defer cleanup{[this] {
		m_Export.Clear();
		ResetExporting(true /* notify all */);
	}};

	namespace ch = std::chrono;

	while (!m_Stopped) {
		m_Export.WaitForSet(yc);
		if (!m_Stream) {
			Connect(yc);
		}

		for (uint64_t attempt = 1; m_Stream; ++attempt) {
			try {
				Export(yc);
				m_Request.reset();
				m_Export.Clear();
				ResetExporting(false /* notify one */);
				break;
			} catch (const RetryableExportError& ex) {
				ch::milliseconds retryAfter;
				if (auto throttle = ex.Throttle(); throttle > 0) {
					retryAfter = ch::milliseconds(ch::seconds(throttle));
				} else {
					retryAfter = Backoff{}(attempt);
				}

				Log(LogWarning, "OTelExporter")
					<< "Failed to export metrics to OpenTelemetry collector (attempt #" << attempt << "). Retrying in "
					<< retryAfter.count() << "ms.";

				boost::system::error_code ec;
				m_RetryExportAndConnTimer.expires_after(retryAfter);
				m_RetryExportAndConnTimer.async_wait(yc[ec]);

				if (m_Stopped) {
					break;
				}
			} catch (const std::exception& ex) {
				LogSeverity severity = LogCritical;
				auto se{dynamic_cast<const boost::system::system_error*>(&ex)};
				// Since we don't have a proper connection health check mechanism, we assume that certain errors
				// indicate a broken connection and force a reconnect in those cases. For the `end_of_stream` case,
				// we downgrade the log severity to debug level since this is a normal occurrence when using an OTEL
				// collector compatible backend that don't honor keep-alive connections (e.g., OpenSearch Data Prepper).
				if (m_Stopped || (se && se->code() == http::error::end_of_stream)) {
					severity = LogDebug;
				}
				Log{severity, "OTelExporter", DiagnosticInformation(ex, false)};
				m_Stream.reset(); // Force reconnect on next export attempt.
			}
		}
	}
}

void OTel::Export(boost::asio::yield_context& yc) const
{
	AsioProtobufOutStream outputS{*m_Stream, m_ConnInfo, yc};
	[[maybe_unused]] auto serialized = m_Request->SerializeToZeroCopyStream(&outputS);
	ASSERT(serialized);
	// Must have completed chunk writing successfully, otherwise reading the response will hang forever.
	VERIFY(outputS.WriterDone());

	IncomingHttpResponse responseMsg{*m_Stream};
	responseMsg.Parse(yc);

	if (auto ct = responseMsg[http::field::content_type]; ct != "application/x-protobuf") {
		if (responseMsg.result() == http::status::ok) {
			// Some OpenTelemetry Collector compatible backends (e.g., Prometheus OTLP Receiver) respond with 200 OK
			// but without the expected Protobuf content type. So, don't do anything here since the request succeeded.
			return;
		}
		Log(LogWarning, "OTelExporter")
			<< "Unexpected Content-Type from OpenTelemetry collector '" << ct << "' (" << responseMsg.reason() << "):\n"
			<< responseMsg.body();
	} else if (responseMsg.result_int() >= 200 && responseMsg.result_int() <= 299) {
		// We've got a valid Protobuf response, so we've to deserialize the body to check for partial success.
		// See https://opentelemetry.io/docs/specs/otlp/#partial-success-1.
		google::protobuf::Arena arena;
		auto response = MetricsResponse::default_instance().New(&arena);
		[[maybe_unused]] auto deserialized = response->ParseFromString(responseMsg.body());
		ASSERT(deserialized);

		if (response->has_partial_success()) {
			const auto& ps = response->partial_success();
			const auto& msg = ps.error_message();
			if (ps.rejected_data_points() > 0 || !msg.empty()) {
				Log(LogWarning, "OTelExporter")
					<< "OpenTelemetry collector reported partial success: " << (msg.empty() ? "<none>" : msg)
					<< " (" << ps.rejected_data_points() << " metric data points rejected).";
			}
		}
	} else if (IsRetryableExportError(responseMsg.result())) {
		uint64_t throttleSeconds = 0;
		try {
			auto throttle = responseMsg[http::field::retry_after];
			// TODO: remove manual string conversion once we drop SLES 15.{6,7} (Boost 1.66 lacks std::string overload).
			throttleSeconds = std::stoull(std::string{throttle.data(), throttle.size()});
		} catch (const std::exception&) {}
		BOOST_THROW_EXCEPTION(RetryableExportError{throttleSeconds});
	} else {
		Log(LogWarning, "OTelExporter")
			<< "OpenTelemetry collector responded with non-success and non-retryable status code "
			<< responseMsg.result_int() << " (" << responseMsg.reason() << ").";
	}
}

/**
 * Reset the exporting state and notify waiters.
 *
 * This method resets the internal exporting state to indicate that no export is currently
 * in progress. It then notifies either one or all waiters waiting for the export to complete,
 * based on the @c notifyAll parameter.
 *
 * @param notifyAll If true, notifies all waiters; otherwise, notifies only one waiter.
 */
void OTel::ResetExporting(bool notifyAll)
{
	{
		std::lock_guard lock(m_Mutex);
		m_Exporting = false;
	}
	if (notifyAll) {
		m_ExportCV.notify_all();
	} else {
		m_ExportCV.notify_one();
	}
}

/**
 * Validate the given OTel metric name according to OTel naming conventions[^1].
 * Here's the ABNF definition for reference:
 * @verbatim
 *   instrument-name = ALPHA 0*254 ("_" / "." / "-" / "/" / ALPHA / DIGIT)
 *   ALPHA = %x41-5A / %x61-7A; A-Z / a-z
 *   DIGIT = %x30-39 ; 0-9
 * @endverbatim
 *
 * @param name The metric name to validate.
 *
 * @throws std::invalid_argument if the metric name is invalid.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otel/metrics/api/#instrument-name-syntax
 */
void OTel::ValidateName(const std::string_view name)
{
	if (name.empty() || name.size() > 255) {
		BOOST_THROW_EXCEPTION(std::invalid_argument("OTel instrument name must be between 1 and 255 characters long."));
	}

	auto isAlpha = [](char c) { return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'); };
	auto isDigit = [](char c) { return '0' <= c && c <= '9'; };
	for (std::size_t i = 0; i < name.size(); ++i) {
		auto c = name[i];
		if (i == 0 && !isAlpha(c)) {
			BOOST_THROW_EXCEPTION(std::invalid_argument("OTel instrument name must start with an alphabetic character."));
		}
		if (!isAlpha(c) && !isDigit(c) && c != '_' && c != '.' && c != '-' && c != '/') {
			BOOST_THROW_EXCEPTION(std::invalid_argument(
				"OTel instrument name contains invalid character '" + std::string(1, c) + "'."
			));
		}
	}
}

/**
 * Set the given OTel attribute key-value pair in the provided @c Attribute Protobuf object.
 *
 * This method sets the given key-value pair in the provided KeyValue Protobuf object according to
 * OTel specifications[^1]. While the OTel specs[^2] allows a wider range of attr value types, we
 * only support the most common/scalar types (Boolean, Number (double), and String) for simplicity.
 *
 * @param attr The OTel attribute Protobuf object to set the value for.
 * @param key The attribute key to set. Must not be empty.
 * @param value The Value object containing the value to set in the attribute.
 *
 * @throws std::invalid_argument if key is empty or if @c Value represents an unsupported attribute value type.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otel/common/#attribute
 * [^2]: https://opentelemetry.io/docs/specs/otel/common/#anyvalue
 */
void OTel::SetAttribute(Attribute& attr, String&& key, Value& value)
{
	if (key.IsEmpty()) {
		BOOST_THROW_EXCEPTION(std::invalid_argument("OTel attribute key must not be empty."));
	}

	switch (value.GetType()) {
		case ValueBoolean:
			attr.mutable_value()->set_bool_value(value.Get<bool>());
			break;
		case ValueNumber:
			attr.mutable_value()->set_double_value(value.Get<double>());
			break;
		case ValueString:
			attr.mutable_value()->set_string_value(std::move(value.Get<String>()));
			break;
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument(
				"OTel attribute value must be of type Boolean, Number, or String, got '" + value.GetTypeName() + "'."
			));
	}
	attr.set_key(std::move(key));
}

/**
 * Record a data point in the given OTel Gauge metric stream with the provided value, timestamps, and attributes.
 *
 * This method adds a new data point to the provided Gauge Protobuf object with the given value, start and end
 * timestamps, and a set of attributes. The value can be either an int64_t or a double, depending on the type
 * of the Gauge. The timestamps are expected to be in seconds and will be converted to nanoseconds as required
 * by OTel specifications. The attributes are provided as a map of key-value pairs and will be set in the data
 * point according to OTel attribute specs.
 *
 * @tparam T The type of the data point value, which must be either int64_t or double.
 *
 * @param gauge The Gauge Protobuf object to record the data point in.
 * @param data The value of the data point to record.
 * @param start The start timestamp of the data point in seconds.
 * @param end The end timestamp of the data point in seconds.
 * @param attrs A map of attribute key-value pairs to set in the data point.
 *
 * @return The size in bytes of the recorded data point after serialization.
 *
 * @throws std::invalid_argument if T is not int64_t or double, or if any attr key is empty or has an unsupported value type.
 */
template<typename T>
[[nodiscard]] std::size_t OTel::Record(Gauge& gauge, T data, double start, double end, AttrsMap&& attrs)
{
	namespace ch = std::chrono;

	auto* dataPoint = gauge.add_data_points();
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

	for (auto it{attrs.begin()}; it != attrs.end(); /* NOPE */) {
		auto* attr = dataPoint->add_attributes();
		auto node = attrs.extract(it++);
		SetAttribute(*attr, std::move(node.key()), node.mapped());
	}
	return dataPoint->ByteSizeLong();
}

/**
 * Determine if the given HTTP status code represents a retryable export error as per OTel specs[^1].
 *
 * @param status The HTTP status code to check.
 *
 * @return true if the status code indicates a retryable error; false otherwise.
 *
 * [^1]: https://opentelemetry.io/docs/specs/otlp/#retryable-response-codes
 */
bool OTel::IsRetryableExportError(const http::status status)
{
	return status == http::status::too_many_requests
		|| status == http::status::bad_gateway
		|| status == http::status::service_unavailable
		|| status == http::status::gateway_timeout;
}

AsioProtobufOutStream::AsioProtobufOutStream(const AsioTlsOrTcpStream& stream, const OTelConnInfo& connInfo, boost::asio::yield_context& yc)
	: m_Writer(stream), m_YieldContext(yc)
{
	m_Writer.method(http::verb::post);
	m_Writer.target(connInfo.MetricsEndpoint);
	m_Writer.set(http::field::host, connInfo.Host + ":" + std::to_string(connInfo.Port));
	m_Writer.set(http::field::content_type, "application/x-protobuf");
	if (connInfo.BasicAuth) {
		m_Writer.set(
			http::field::authorization,
			"Basic " + Base64::Encode(connInfo.BasicAuth->Get("username") + ":" + connInfo.BasicAuth->Get("password"))
		);
	}
	m_Writer.StartStreaming();
}

bool AsioProtobufOutStream::Next(void** data, int* size)
{
	if (m_Buffered == l_BufferSize) {
		Flush();
	}
	// Prepare a new buffer segment that the Protobuf serializer can write into.
	// The buffer size is fixed to l_BufferSize, and as seen above, we flush if the previous buffer
	// segment was fully used (which is always the case on each Next call after the initial one), so
	// we'll end up reusing the same memory region for each Next call because when we flush, we also
	// consume the committed data, and that region becomes writable again.
	auto buf = m_Writer.Prepare(l_BufferSize - m_Buffered);
	*data = buf.data();
	*size = static_cast<int>(l_BufferSize);
	m_Buffered = l_BufferSize;
	return true;
}

void AsioProtobufOutStream::BackUp(int count)
{
	// Make sure we've not already finalized the HTTP body because BackUp
	// is supposed to be called only after a preceding (final) Next call.
	ASSERT(!m_Writer.Done());
	ASSERT(static_cast<int64_t>(count) <= m_Buffered);
	ASSERT(m_Buffered == l_BufferSize);
	// If the last prepared buffer segment was not fully used, we need to adjust the buffered size,
	// so that we don't commit unused memory regions with the below Flush() call. If count is zero,
	// this adjustment is a no-op, and indicates that the entire buffer was used and there won't be
	// any subsequent Next calls anymore (i.e., the Protobuf serialization is complete).
	m_Buffered -= count;
	Flush(true);
}

int64_t AsioProtobufOutStream::ByteCount() const
{
	return m_Pos + m_Buffered;
}

/**
 * Flush any buffered data to the underlying Asio stream.
 *
 * If the `finish` parameter is set to true, it indicates that no more data will
 * be buffered/generated, and the HTTP body will be finalized accordingly.
 *
 * @param finish Whether this is the final flush operation.
 */
void AsioProtobufOutStream::Flush(bool finish)
{
	ASSERT(m_Buffered > 0 || finish);
	m_Writer.Commit(m_Buffered);
	m_Writer.Flush(m_YieldContext, finish);
	m_Pos += m_Buffered;
	m_Buffered = 0;
}

/**
 * Check if the underlying HTTP request writer has completed writing.
 *
 * @return true if the writer has finished writing; false otherwise.
 */
bool AsioProtobufOutStream::WriterDone()
{
	return m_Writer.Done();
}
