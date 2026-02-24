// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/dictionary.hpp"
#include "base/io-engine.hpp"
#include "base/json.hpp"
#include "base/tlsstream.hpp"
#include "remote/apiuser.hpp"
#include "remote/httpserverconnection.hpp"
#include "remote/url.hpp"
#include <boost/beast/http.hpp>
#include <boost/version.hpp>
#include <memory>
#include <utility>

namespace icinga {

/**
 * A custom body_type for a  @c boost::beast::http::message
 *
 * It combines the memory management of @c boost::beast::http::dynamic_body,
 * which uses a multi_buffer, with the ability to continue serialization when
 * new data arrives of the @c boost::beast::http::buffer_body.
 *
 * @tparam DynamicBuffer A buffer conforming to the boost::beast interface of the same name
 *
 * @ingroup remote
 */
template<class DynamicBuffer>
struct SerializableBody
{
	class writer;

	class value_type
	{
	public:
		template<typename T>
		value_type& operator<<(T&& right)
		{
			/* Preferably, we would return an ostream object here instead. However
			 * there seems to be a bug in boost::beast where if the ostream, or rather its
			 * streambuf object is moved into the return value, the chunked encoding gets
			 * mangled, leading to the client disconnecting.
			 *
			 * A workaround would have been to construct the boost::beast::detail::ostream_helper
			 * with the last parameter set to false, indicating that the streambuf object is not
			 * movable, but that is an implementation detail we'd rather not use directly in our
			 * code.
			 *
			 * This version has a certain overhead of the ostream being constructed on every call
			 * to the operator, which leads to an individual append for each time, whereas if the
			 * object could be kept until the entire chain of output operators is finished, only
			 * a single call to prepare()/commit() would have been needed.
			 *
			 * However, since this operator is mostly used for small error messages and the big
			 * responses are handled via a reader instance, this shouldn't be too much of a
			 * problem.
			 */
			boost::beast::ostream(m_Buffer) << std::forward<T>(right);
			return *this;
		}

		[[nodiscard]] std::size_t Size() const { return m_Buffer.size(); }

		void Finish() { m_More = false; }
		bool Finished() { return !m_More; }
		void Start() { m_More = true; }
		DynamicBuffer& Buffer() { return m_Buffer; }

		friend class writer;

	private:
		/* This defaults to false so the body does not require any special handling
		 * for simple messages and can still be written with http::async_write().
		 */
		bool m_More = false;
		DynamicBuffer m_Buffer;
	};

	static std::uint64_t size(const value_type& body) { return body.Size(); }

	/**
	 * Implement the boost::beast BodyWriter interface for this body type
	 *
	 * This is used (for example) by the @c boost::beast::http::serializer to write out the
	 * message over the TLS stream. The logic is similar to the writer of the
	 * @c boost::beast::http::buffer_body.
	 *
	 * On the every call, it will free up the buffer range that has previously been written,
	 * then return a buffer containing data the has become available in the meantime. Otherwise,
	 * if there is more data expected in the future, for example because a corresponding reader
	 * has not yet finished filling the body, a `need_buffer` error is returned, to inform the
	 * serializer to abort writing for now, which in turn leads to the outer call to
	 * `http::async_write` to call their completion handlers with a `need_buffer` error, to
	 * notify that more data is required for another call to `http::async_write`.
	 */
	class writer
	{
	public:
		using const_buffers_type = typename DynamicBuffer::const_buffers_type;

#if BOOST_VERSION > 106600
		template<bool isRequest, class Fields>
		explicit writer(const boost::beast::http::header<isRequest, Fields>&, value_type& b) : m_Body(b)
		{
		}
#else
		/**
		 * This constructor is needed specifically for boost-1.66, which was the first version
		 * the beast library was introduced and is still used on older (supported) distros.
		 */
		template<bool isRequest, class Fields>
		explicit writer(const boost::beast::http::message<isRequest, SerializableBody, Fields>& msg)
			: m_Body(const_cast<value_type&>(msg.body()))
		{
		}
#endif
		void init(boost::beast::error_code& ec) { ec = {}; }

		boost::optional<std::pair<const_buffers_type, bool>> get(boost::beast::error_code& ec)
		{
			using namespace boost::beast::http;

			if (m_SizeWritten > 0) {
				m_Body.m_Buffer.consume(std::exchange(m_SizeWritten, 0));
			}

			if (m_Body.m_Buffer.size()) {
				ec = {};
				m_SizeWritten = m_Body.m_Buffer.size();
				return {{m_Body.m_Buffer.data(), m_Body.m_More}};
			}

			if (m_Body.m_More) {
				ec = {make_error_code(error::need_buffer)};
			} else {
				ec = {};
			}
			return boost::none;
		}

	private:
		value_type& m_Body;
		std::size_t m_SizeWritten = 0;
	};
};

using SerializableMultiBufferBody = SerializableBody<boost::beast::multi_buffer>;
using SerializableFlatBufferBody = SerializableBody<boost::beast::flat_buffer>;

template<bool isRequest, typename Body, typename StreamVariant>
class IncomingHttpMessage : public boost::beast::http::message<isRequest, Body>
{
	using ParserType = boost::beast::http::parser<isRequest, Body>;
	using Base = boost::beast::http::message<isRequest, Body>;

public:
	explicit IncomingHttpMessage(StreamVariant stream);

	/**
	 * Parse the header of the response using the internal parser object.
	 *
	 * This first performs an @f async_read_header() into the parser, then copies
	 * the parsed header into this object.
	 */
	void ParseHeader(boost::beast::flat_buffer& buf, boost::asio::yield_context yc);

	/**
	 * Parse the body of the response using the internal parser object.
	 *
	 * This first performs an async_read() into the parser, then moves the parsed body
	 * into this object.
	 *
	 * @param buf The buffer used to track the state of the connection
	 * @param yc The yield_context for this operation
	 */
	void ParseBody(boost::beast::flat_buffer& buf, boost::asio::yield_context yc);

	ParserType& Parser() { return m_Parser; }

private:
	ParserType m_Parser;

	StreamVariant m_Stream;
};

using IncomingHttpRequest = IncomingHttpMessage<true, boost::beast::http::string_body, AsioTlsOrTcpStream>;
using IncomingHttpResponse = IncomingHttpMessage<false, boost::beast::http::string_body, AsioTlsOrTcpStream>;

template<bool isRequest, typename Body, typename StreamVariant>
class OutgoingHttpMessage : public boost::beast::http::message<isRequest, Body>
{
	using Serializer = boost::beast::http::serializer<isRequest, Body>;
	using Base = boost::beast::http::message<isRequest, Body>;

public:
	explicit OutgoingHttpMessage(StreamVariant stream);

	/* Delete the base class clear() which is inherited from the fields<> class and doesn't
	 * clear things like the body or obviously our own members.
	 */
	void clear() = delete;

	/**
	 * Clear the header and body of the message.
	 *
	 * @note This can only be used when nothing has been written to the stream yet.
	 */
	void Clear();

	/**
	 * Commits the specified number of bytes (previously obtained via @c prepare()) for reading.
	 *
	 * This function makes the specified number of bytes available in the body buffer for reading.
	 *
	 * @param size The number of bytes to commit
	 */
	void Commit(std::size_t size) { Base::body().Buffer().commit(size); }

	/**
	 * Prepare a buffer of the specified size for writing.
	 *
	 * The returned buffer serves just as a view onto the internal buffer sequence but does not actually
	 * own the memory. Thus, destroying the returned buffer will not free any memory it represents.
	 *
	 * @param size The size of the buffer to prepare
	 *
	 * @return A mutable buffer representing the prepared space
	 */
	auto Prepare(std::size_t size) { return Base::body().Buffer().prepare(size); }

	/**
	 * Enables chunked encoding.
	 */
	void StartStreaming();

	/**
	 * Writes as much of the response as is currently available.
	 *
	 * Uses chunk-encoding if the content_length has not been set by the time this is called
	 * for the first time.
	 *
	 * The caller needs to ensure that the header is finished before calling this for the
	 * first time as changes to the header afterwards will not have any effect.
	 *
	 * @param yc The yield_context for this operation
	 */
	void Flush(boost::asio::yield_context yc, bool finish = false);

	[[nodiscard]] bool HasSerializationStarted() const { return m_SerializationStarted; }

	/**
	 * Sends the contents of a file.
	 *
	 * This does not use chunked encoding because the file size is expected to be fixed.
	 * The message will be flushed to the stream after a certain amount has been loaded into
	 * the buffer.
	 *
	 * @todo Switch the implementation to @c boost::asio::stream_file when we require >=boost-1.78.
	 *
	 * @param path A path to the file
	 * @param yc The yield context for flushing the message.
	 */
	void SendFile(const String& path, const boost::asio::yield_context& yc);

	JsonEncoder GetJsonEncoder(bool pretty = false);

private:
	Serializer m_Serializer{*this};
	bool m_SerializationStarted = false;

	StreamVariant m_Stream;
};

using OutgoingHttpRequest = OutgoingHttpMessage<true, SerializableFlatBufferBody, AsioTlsOrTcpStream>;
using OutgoingHttpResponse = OutgoingHttpMessage<false, SerializableFlatBufferBody, AsioTlsOrTcpStream>;

class HttpApiRequest
	: public IncomingHttpMessage<true, boost::beast::http::string_body, std::variant<Shared<AsioTlsStream>::Ptr>>
{
public:
	explicit HttpApiRequest(Shared<AsioTlsStream>::Ptr stream);

	[[nodiscard]] ApiUser::Ptr User() const;
	void User(const ApiUser::Ptr& user);

	[[nodiscard]] icinga::Url::Ptr Url() const;
	void DecodeUrl();

	[[nodiscard]] Dictionary::Ptr Params() const;
	void DecodeParams();

private:
	ApiUser::Ptr m_User;
	Url::Ptr m_Url;
	Dictionary::Ptr m_Params;
};

/**
 * A wrapper class for a boost::beast HTTP response for the Icinga 2 API
 *
 * @ingroup remote
 */
class HttpApiResponse
	: public OutgoingHttpMessage<false, SerializableMultiBufferBody, std::variant<Shared<AsioTlsStream>::Ptr>>
{
public:
	explicit HttpApiResponse(Shared<AsioTlsStream>::Ptr stream, HttpServerConnection::Ptr server = nullptr);

	/**
	 * Enables chunked encoding.
	 *
	 * Optionally starts a coroutine that reads from the stream and checks for client-side
	 * disconnects. In this case, the stream can not be reused after the response has been
	 * sent and any further requests sent over the connections will be discarded, even if
	 * no client-side disconnect occurs. This requires that this object has been constructed
	 * with a valid HttpServerConnection::Ptr.
	 *
	 * @param checkForDisconnect Whether to start a coroutine to detect disconnects
	 */
	void StartStreaming(bool checkForDisconnect);

	/**
	 * Check if the server has initiated a disconnect.
	 *
	 * @note This requires that the message has been constructed with a pointer to the
	 * @c HttpServerConnection.
	 */
	[[nodiscard]] bool IsClientDisconnected() const;

	void SetCpuBoundWork(std::weak_ptr<CpuBoundWork> cbw)
	{
		m_CpuBoundWork = std::move(cbw);
	}

private:
	HttpServerConnection::Ptr m_Server;
	std::weak_ptr<CpuBoundWork> m_CpuBoundWork;
};

// More general instantiations
extern template class OutgoingHttpMessage<true, SerializableFlatBufferBody, AsioTlsOrTcpStream>;
extern template class OutgoingHttpMessage<false, SerializableFlatBufferBody, AsioTlsOrTcpStream>;
extern template class IncomingHttpMessage<true, boost::beast::http::string_body, AsioTlsOrTcpStream>;
extern template class IncomingHttpMessage<false, boost::beast::http::string_body, AsioTlsOrTcpStream>;

// Instantiations specifically for HttpApi(Request|Response)
extern template class IncomingHttpMessage<true, boost::beast::http::string_body, std::variant<Shared<AsioTlsStream>::Ptr>>;
extern template class OutgoingHttpMessage<false, SerializableMultiBufferBody, std::variant<Shared<AsioTlsStream>::Ptr>>;

} // namespace icinga
