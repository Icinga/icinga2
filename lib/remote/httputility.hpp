/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include "base/tlsstream.hpp"
#include "remote/url.hpp"
#include "remote/apiuser.hpp"
#include "base/io-engine.hpp"
#include "base/dictionary.hpp"
#include <boost/beast/http.hpp>
#include <boost/signals2.hpp>
#include <string>

namespace icinga
{

/**
 * A custom body_type for a boost::beast::http::message
 *
 * It combines the memory management of dynamic_body, which uses a multi_buffer,
 * with the ability to continue serialization when new data arrives of the buffer_body.
 *
 * Additionally, it also adds an AsioEvent to signal that more data is available and the
 * serialization can be continued.
 */
struct serializable_dynamic_body
{
	class reader;
	class writer;
	
	// TODO: Maybe make variables private
	class value_type
	{
	public:
		bool more = false;
		AsioEvent moreData{IoEngine::Get().GetIoContext()};
		boost::beast::multi_buffer buffer;
		template <typename T>
		value_type & operator<<(const T& right)
		{
			boost::beast::ostream(buffer) << right;
			moreData.Set();
			return *this;
		}

		size_t size() const { return buffer.size(); }

		friend class reader;
		friend class writer;
	};

	static std::uint64_t size(value_type const& body) { return body.size(); }

	class reader : boost::beast::http::dynamic_body::reader
	{
		value_type& body;

	public:
		template <bool isRequest, class Fields>
		reader(boost::beast::http::header<isRequest, Fields> & h, value_type& b)
			: body(b), boost::beast::http::dynamic_body::reader(h, b.buffer)
		{
			body.more = true;
		}

		void init(boost::optional<std::uint64_t> const&, boost::beast::error_code& ec) { ec = {}; }

		template <class ConstBufferSequence>
		std::size_t put(ConstBufferSequence const& buffers, boost::beast::error_code& ec)
		{
			using namespace boost::beast::http;
			body.moreData.Set();
			return dynamic_body::reader::put(buffers, ec);
		}
		
        void finish(boost::beast::error_code& ec)
        {
			ec = {};
			body.more = false;
        }
	};

	class writer
	{
		value_type& body;
		bool toggle = false;

	public:
		using const_buffers_type = typename decltype(value_type::buffer)::const_buffers_type;

		template <bool isRequest, class Fields>
		writer(boost::beast::http::header<isRequest, Fields> const& h, value_type& b) : body(b)
		{
		}

		void init(boost::beast::error_code& ec) { ec = {}; }

		boost::optional<std::pair<const_buffers_type, bool>> get(boost::beast::error_code& ec)
		{
			using namespace boost::beast::http;

			if (toggle) {
				toggle = false;
				body.moreData.Clear();
				body.buffer.consume(body.buffer.size());
			} else if (body.buffer.size()) {
				ec = {};
				toggle = true;
				return {{body.buffer.data(), body.more}};
			}

			if (body.more) {
				ec = {make_error_code(error::need_buffer)};
			} else {
				ec = {};
			}
			return boost::none;
		}
	};
};

class HttpRequest: public boost::beast::http::request<boost::beast::http::string_body>
{
public:
	using base_type = boost::beast::http::request<boost::beast::http::string_body>;

	HttpRequest(boost::beast::http::request<boost::beast::http::string_body> && other);

	const ApiUser::Ptr& User();
	void User(const ApiUser::Ptr& user);
	void FetchUser();

	const icinga::Url::Ptr& Url();

	const Dictionary::Ptr& Params() const;
	void DecodeParams();

	Value GetLastParameter(const String& key) const;

private:
	ApiUser::Ptr user;
	Url::Ptr url;
	Dictionary::Ptr params;
};

class HttpResponse: public boost::beast::http::response<serializable_dynamic_body>
{
	bool m_WriteError = false;
	AsioEvent m_WriteReady;
	AsioEvent m_WriteDone;
	
public:
	using base_type = boost::beast::http::response<HttpResponse::body_type>;
	using Serializer = boost::beast::http::response_serializer<HttpResponse::body_type>;

	HttpResponse();

	void Begin();
	//TODO: This becomes useless when all
	void Done();

	void Write(AsioTlsStream& stream, boost::asio::yield_context& yc);
	void Wait(boost::asio::yield_context& yc);

	bool WriteError() const { return m_WriteError; }

	//TODO: Replace these with something more convenient
	void SendJsonBody(const Dictionary::Ptr& params, const Value& val);
	void SendJsonError(const Dictionary::Ptr& params, const int code, const String& verbose = String(),
		const String& diagnosticInformation = String());

};

/**
 * Helper functions.
 *
 * @ingroup remote
 */
class HttpUtility
{
public:
	static Value GetLastParameter(const Dictionary::Ptr& params, const String& key);
};

}

#endif /* HTTPUTILITY_H */
