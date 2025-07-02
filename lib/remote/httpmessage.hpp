/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/dictionary.hpp"
#include "base/json.hpp"
#include "base/tlsstream.hpp"
#include "remote/apiuser.hpp"
#include "remote/httpserverconnection.hpp"
#include "remote/url.hpp"
#include <boost/beast/http.hpp>
#include <boost/version.hpp>

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

/**
 * A wrapper class for a boost::beast HTTP request
 *
 * @ingroup remote
 */
class HttpRequest : public boost::beast::http::request<boost::beast::http::string_body>
{
public:
	using ParserType = boost::beast::http::request_parser<body_type>;

	explicit HttpRequest(Shared<AsioTlsStream>::Ptr stream);

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

	ParserType m_Parser;

	Shared<AsioTlsStream>::Ptr m_Stream;
};

/**
 * A wrapper class for a boost::beast HTTP response
 *
 * @ingroup remote
 */
class HttpResponse : public boost::beast::http::response<SerializableBody<boost::beast::multi_buffer>>
{
public:
	explicit HttpResponse(Shared<AsioTlsStream>::Ptr stream, HttpServerConnection::Ptr server = nullptr);

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
	void Flush(boost::asio::yield_context yc);

	[[nodiscard]] bool HasSerializationStarted() const { return m_SerializationStarted; }

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
	void StartStreaming(bool checkForDisconnect = false);

	/**
	 * Check if the server has initiated a disconnect.
	 *
	 * @note This requires that the message has been constructed with a pointer to the
	 * @c HttpServerConnection.
	 */
	[[nodiscard]] bool IsClientDisconnected() const;

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
	using Serializer = boost::beast::http::response_serializer<HttpResponse::body_type>;
	Serializer m_Serializer{*this};
	bool m_SerializationStarted = false;

	HttpServerConnection::Ptr m_Server;
	Shared<AsioTlsStream>::Ptr m_Stream;
};

} // namespace icinga
