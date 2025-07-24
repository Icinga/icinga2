/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include "base/dictionary.hpp"
#include "base/tlsstream.hpp"
#include "base/json.hpp"
#include "remote/url.hpp"
#include "remote/apiuser.hpp"
#include <boost/beast/http.hpp>
#include <boost/asio/streambuf.hpp>

namespace icinga
{

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

private:
	ApiUser::Ptr m_User;
	Url::Ptr m_Url;
	Dictionary::Ptr m_Params;

	ParserType m_Parser;

	Shared<AsioTlsStream>::Ptr m_Stream;
};

/**
 * A wrapper class for a boost::beast HTTP response.
 *
 * This is a convenience class to handle HTTP responses in a more streaming-like way. This class is derived
 * from @c boost::beast::http::response<boost::beast::http::empty_body> and as its name implies, it doesn't
 * and won't contain a body. Instead, it uses its own internal buffer to efficiently manage the response body
 * data, which can be filled using the output stream operator (`<<`) or the @c JsonEncoder class. This allows
 * for a more flexible and efficient way to handle HTTP responses, without having to copy Asio/Beast buffers
 * around. It uses a @c boost::asio::streambuf internally to manage the response body data, which is suitable
 * for the @c JsonEncoder class to write JSON data directly into it and then forward it as-is to Asio's
 * @c boost::asio::async_write() function.
 *
 * Furthermore, it supports chunked encoding as well as fixed-size responses. If chunked encoding is used, the
 * @c m_Buffer is written in chunks created by the @c boost::beast::http::make_chunk() function, which doesn't
 * copy the buffer data, but instead creates a view similar to @c std::string_view but for buffers and allows
 * for efficient streaming of the response body data. Once you've finished writing to the response, you must
 * call the @c Finish() followed by a call to @c Flush() to indicate that there will be no more data to stream
 * and to write the final chunk with a size of zero to indicate the end of the response. If you don't call @c Finish(),
 * the response will not be flushed and the client will not receive the final chunk, which may lead to a timeout
 * or other issues on the client side.
 *
 * @ingroup remote
 */
class HttpResponse : public boost::beast::http::response<boost::beast::http::empty_body>
{
public:
	explicit HttpResponse(Shared<AsioTlsStream>::Ptr stream);

	std::size_t Size() const;
	bool IsHeaderDone() const;

	void Flush(boost::asio::yield_context yc);

	void StartStreaming();

	void Finish()
	{
		m_Finished = true;
	}

	JsonEncoder GetJsonEncoder(bool pretty = false);

	/**
	 * Write data to the response body.
	 *
	 * This function appends the given data to the response body using the internal output stream.
	 * This is the only available way to fill the response body with data and must not be called
	 * after the response has been finished (i.e., after calling @c Finish()).
	 *
	 * @tparam T The type of the data to write. It can be any type that supports the output stream operator (`<<`).
	 *
	 * @param data The data to write to the response body
	 *
	 * @return Reference to this HttpResponse object for chaining operations.
	 */
	template<typename T>
	HttpResponse& operator<<(T&& data)
	{
		ASSERT(!m_Finished); // Never allow writing to an already finished response.
		m_Ostream << std::forward<T>(data);
		return *this;
	}

private:
	bool m_HeaderDone = false; // Indicates if the response headers have been written to the stream.
	bool m_Finished = false; // Indicates if the response is finished and no more data will be written.

	boost::asio::streambuf m_Buffer; // The buffer used to store the response body.
	std::ostream m_Ostream{&m_Buffer}; // The output stream used to write data to the response body.

	Shared<AsioTlsStream>::Ptr m_Stream;
};

}

#endif /* HTTPUTILITY_H */
