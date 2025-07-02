/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include "base/dictionary.hpp"
#include "base/tlsstream.hpp"
#include "base/json.hpp"
#include "remote/url.hpp"
#include "remote/apiuser.hpp"
#include <boost/beast/http.hpp>
#include <boost/version.hpp>

namespace icinga
{

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
	class reader;
	class writer;

	class value_type
	{
	public:
		template <typename T>
		value_type& operator<<(T && right)
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

		std::size_t Size() const { return m_Buffer.size(); }

		void Finish() { m_More = false; }
		void Start() { m_More = true; }

		friend class reader;
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
	 * Implement the boost::beast BodyReader interface for this body type
	 *
	 * This is used by the  @c boost::beast::http::parser (which we don't use for responses)
	 * or in the  @c BeastHttpMessageAdapter for our own  @c JsonEncoder to write JSON directly
	 * into the body of a HTTP response message.
	 *
	 * The reader automatically sets the body's `more` flag on the call to its init() function
	 * and resets it when its finish() function is called. Regarding the usage in the
	 * @c JsonEncoder above, this means that the message is automatically marked as complete
	 * when the encoder object is destroyed.
	 */
	class reader
	{
	public:
		template <bool isRequest, class Fields>
		explicit reader(boost::beast::http::header<isRequest, Fields>& h, value_type& b) : m_Body(b)
		{
		}

		void init(const boost::optional<std::uint64_t>& n, boost::beast::error_code& ec)
		{
			ec = {};
			m_Body.Start();
#if BOOST_VERSION >= 107000
			if (n) {
				m_Body.m_Buffer.reserve(*n);
			}
#endif /* BOOST_VERSION */
		}

		template <class ConstBufferSequence>
		std::size_t put(const ConstBufferSequence& buffers, boost::beast::error_code& ec)
		{
			auto size = boost::asio::buffer_size(buffers);
			if (size > m_Body.m_Buffer.max_size() - m_Body.Size()) {
				ec = boost::beast::http::error::buffer_overflow;
				return 0;
			}

			auto const wBuf = m_Body.m_Buffer.prepare(size);
			boost::asio::buffer_copy(wBuf, buffers);
			m_Body.m_Buffer.commit(size);
			return size;
		}

		void finish(boost::beast::error_code& ec)
		{
			ec = {};
			m_Body.Finish();
		}

	private:
		value_type& m_Body;
	};

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
		using const_buffers_type = typename decltype(value_type::m_Buffer)::const_buffers_type;

		template <bool isRequest, class Fields>
		explicit writer(const boost::beast::http::header<isRequest, Fields>& h, value_type& b)
			: m_Body(b)
		{
		}

		/**
		 * This constructor is needed specifically for boost-1.66, which was the first version
		 * the beast library was introduced and is still used on older (supported) distros.
		 */
		template <bool isRequest, class Fields>
		explicit writer(const boost::beast::http::message<isRequest, SerializableBody, Fields>& msg)
			: m_Body(const_cast<value_type &>(msg.body()))
		{
		}

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
class HttpRequest: public boost::beast::http::request<boost::beast::http::string_body>
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

	ApiUser::Ptr User() const;
	void User(const ApiUser::Ptr& user);

	icinga::Url::Ptr Url() const;
	void DecodeUrl();

	Dictionary::Ptr Params() const;
	void DecodeParams();

	Value GetLastParameter(const String& key) const;

	/**
	 * Return true if pretty printing was requested.
	 */
	bool IsPretty() const;

	/**
	 * Return true if a verbose response was requested.
	 */
	bool IsVerbose() const;

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
class HttpResponse: public boost::beast::http::response<SerializableBody<boost::beast::multi_buffer>>
{
public:
	explicit HttpResponse(Shared<AsioTlsStream>::Ptr stream);

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

	bool HasSerializationStarted () { return m_Serializer.is_header_done(); }

	/**
	 * Enables chunked encoding.
	 */
	void StartStreaming();

	void SendJsonBody(const Value& val, bool pretty = false);
	void SendJsonError(const int code, String info = String(), String diagInfo = String(),
		bool pretty = false, bool verbose = false);
	void SendJsonError(const Dictionary::Ptr& params, const int code,
		String info = String(), String diagInfo = String());

	JsonEncoder GetJsonEncoder(bool pretty = false);
	
private:
	using Serializer = boost::beast::http::response_serializer<HttpResponse::body_type>;
	Serializer m_Serializer{*this};

	Shared<AsioTlsStream>::Ptr m_Stream;
};

}

#endif /* HTTPUTILITY_H */
