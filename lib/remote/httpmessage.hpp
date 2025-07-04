/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include "base/dictionary.hpp"
#include "base/tlsstream.hpp"
#include "remote/url.hpp"
#include "remote/apiuser.hpp"
#include <boost/beast/http.hpp>

namespace icinga
{

/**
 * A custom body_type for a  @c boost::beast::http::message
 *
 * It combines the memory management of @c boost::beast::http::dynamic_body,
 * which uses a multi_buffer, with the ability to continue serialization when
 * new data arrives of the @c boost::beast::http::buffer_body.
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
		value_type& operator<<(const T& right)
		{
			boost::beast::ostream(buffer) << right;
			return *this;
		}

		size_t Size() const { return buffer.size(); }

		void Finish() { more = false; }

		friend class reader;
		friend class writer;

	private:
		/* This defaults to false so the body does not require any special handling
		 * for simple messages and can still be written with http::async_write().
		 */
		bool more = false;
		DynamicBuffer buffer;
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
		explicit reader(boost::beast::http::header<isRequest, Fields>& h, value_type& b) : body(b)
		{
		}

		void init(const boost::optional<std::uint64_t>& n, boost::beast::error_code& ec)
		{
			ec = {};
			body.more = true;
			if (n) {
				body.buffer.reserve(*n);
			}
		}

		template <class ConstBufferSequence>
		std::size_t put(const ConstBufferSequence& buffers, boost::beast::error_code& ec)
		{
			auto size = boost::asio::buffer_size(buffers);
			if (size > body.buffer.max_size() - body.Size()) {
				ec = boost::beast::http::error::buffer_overflow;
				return 0;
			}

			auto const wBuf = body.buffer.prepare(size);
			boost::asio::buffer_copy(wBuf, buffers);
			body.buffer.commit(size);
			return size;
		}

		void finish(boost::beast::error_code& ec)
		{
			ec = {};
			body.more = false;
		}

	private:
		value_type& body;
	};

	/**
	 * Implement the boost::beast BodyWriter interface for this body type
	 *
	 * This is used (for example) by the @c boost::beast::http::serializer to write out the
	 * message over the TLS stream. The logic is similar to the writer of the
	 * @c boost::beast::http::buffer_body.
	 *
	 * On the first call, the get method will return the entire buffer and set a toggle.
	 *
	 * On the second call, it will free the buffer range returned previously. Also, if there
	 * is more data expected, for example because a corresponding reader has not yet finished
	 * filling out the body, a `need_buffer` error is returned, informing the serializer to
	 * abort writing without finishing, which in turn leads to the outer call to
	 * `http::async_write` calling their completion handlers with the `need_buffer` error,
	 * notifying that more data is required for another call to `http::async_write`.
	 */
	class writer
	{
	public:
		using const_buffers_type = typename decltype(value_type::buffer)::const_buffers_type;

		template <bool isRequest, class Fields>
		explicit writer(const boost::beast::http::header<isRequest, Fields>& h, value_type& b)
			: body(b)
		{
		}

		/**
		 * This constructor is needed specifically for boost-1.66, which was the first version
		 * the beast library was introduced and is still used on older (supported) distros.
		 */
		template <bool isRequest, class Fields>
		explicit writer(const boost::beast::http::message<isRequest, SerializableBody, Fields>& msg)
			: body(const_cast<value_type &>(msg.body()))
		{
		}

		void init(boost::beast::error_code& ec) { ec = {}; }

		boost::optional<std::pair<const_buffers_type, bool>> get(boost::beast::error_code& ec)
		{
			using namespace boost::beast::http;

			if (sizeWritten > 0) {
				body.buffer.consume(std::exchange(sizeWritten, 0));
			}

			if (body.buffer.size()) {
				ec = {};
				sizeWritten = body.buffer.size();
				return {{body.buffer.data(), body.more}};
			}

			if (body.more) {
				ec = {make_error_code(error::need_buffer)};
			} else {
				ec = {};
			}
			return boost::none;
		}

	private:
		value_type& body;
		decltype(body.buffer.size()) sizeWritten = 0;
	};
};

class HttpRequest: public boost::beast::http::request<boost::beast::http::string_body>
{
public:
	using ParserType = boost::beast::http::request_parser<body_type>;

	HttpRequest(Shared<AsioTlsStream>::Ptr stream);

	/**
	 * Parse the header of the repsonse using the internal parser object.
	 *
	 * This first performs an async_read_header, then copies the header into this object.
	 */
	void ParseHeader(boost::beast::flat_buffer & buf, boost::asio::yield_context yc);
	void ParseBody(boost::beast::flat_buffer& buf, boost::asio::yield_context yc);
	ParserType& Parser() { return m_Parser; }

	const ApiUser::Ptr& User();
	void User(const ApiUser::Ptr& user);

	const icinga::Url::Ptr& Url();

	const Dictionary::Ptr& Params() const;
	void DecodeParams();
	bool IsPretty();
	bool IsVerbose();

	Value GetLastParameter(const String& key) const;

private:
	ApiUser::Ptr m_User;
	Url::Ptr m_Url;
	Dictionary::Ptr m_Params;

	ParserType m_Parser;

	Shared<AsioTlsStream>::Ptr m_Stream;
};

class HttpResponse: public boost::beast::http::response<SerializableBody<boost::beast::multi_buffer>>
{
public:
	using string_view = boost::beast::string_view;
	HttpResponse(Shared<AsioTlsStream>::Ptr stream);

	/**
	 * Writes as much of the response as is currently available.
	 *
	 * Uses chunk-encoding if the content_length has not been set by the time this is called
	 * for the first time.
	 *
	 * The caller needs to ensure that the header is finished before calling this for the
	 * first time as changes to the header afterwards will not have any effect.
	 *
	 * @param yc The yield_context
	 */
	void Write(boost::asio::yield_context& yc);

	bool IsWritable() const { return m_Stream->lowest_layer().is_open(); }

	void SendJsonBody(const Value& val, bool pretty = false);
	void SendJsonError(const int code, String info = String(), String diagInfo = String(),
		bool pretty = false, bool verbose = false);
	void SendJsonError(const Dictionary::Ptr& params, const int code,
		String info = String(), String diagInfo = String());

private:
	using Serializer = boost::beast::http::response_serializer<HttpResponse::body_type>;
	Serializer m_Serializer{*this};
	bool m_HeaderDone = false;

	Shared<AsioTlsStream>::Ptr m_Stream;
};

}

#endif /* HTTPUTILITY_H */
