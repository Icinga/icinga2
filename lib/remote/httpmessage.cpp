/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "remote/httpmessage.hpp"
#include "remote/httputility.hpp"
#include "remote/url.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include <map>
#include <string>
#include <boost/beast/http.hpp>

using namespace icinga;

HttpRequest::HttpRequest(Shared<AsioTlsStream>::Ptr stream)
	: m_Stream(std::forward<decltype(m_Stream)>(stream)){}

void HttpRequest::ParseHeader(boost::beast::flat_buffer & buf, boost::asio::yield_context yc)
{
	boost::beast::http::async_read_header(*m_Stream, buf, m_Parser, yc);
	base() = m_Parser.get().base();
}

void HttpRequest::ParseBody(boost::beast::flat_buffer & buf, boost::asio::yield_context yc)
{
	boost::beast::http::async_read(*m_Stream, buf, m_Parser, yc);
	body() = std::move(m_Parser.release().body());
}

const ApiUser::Ptr& HttpRequest::User()
{
	return m_User;
}

void HttpRequest::User(const ApiUser::Ptr& user)
{
	m_User = user;
}

const Url::Ptr& HttpRequest::Url()
{
	if (!m_Url) {
		m_Url = new icinga::Url(std::string(target()));
	}
	return m_Url;
}

const Dictionary::Ptr& HttpRequest::Params() const
{
	return m_Params;
}

void HttpRequest::DecodeParams()
{
	if (!body().empty()) {
		Log(LogDebug, "HttpUtility")
			<< "Request body: '" << body() << '\'';

		m_Params = JsonDecode(body());
	}

	if (!m_Params)
		m_Params = new Dictionary();

	std::map<String, ArrayData> query;
	for (const auto& kv : Url()->GetQuery()) {
		query[kv.first].emplace_back(kv.second);
	}

	for (auto& kv : query) {
		m_Params->Set(kv.first, new Array{kv.second});
	}
}

bool HttpRequest::IsPretty()
{
	return GetLastParameter("pretty");
}

bool HttpRequest::IsVerbose()
{
	return GetLastParameter("verbose");
}

Value HttpRequest::GetLastParameter(const String& key) const
{
	return HttpUtility::GetLastParameter(Params(), key);
}

HttpResponse::HttpResponse(Shared<AsioTlsStream>::Ptr stream)
	: m_Stream(std::move(stream))
{
}

void HttpResponse::Write(boost::asio::yield_context& yc)
{
	if (!std::exchange(m_HeaderDone, true)) {
		if (!has_content_length()) {
			chunked(true);
		}
	}

	if (body().Size()) {
		boost::system::error_code ec;
		boost::beast::http::async_write(*m_Stream, m_Serializer, yc[ec]);
		if (ec && ec != boost::beast::http::error::need_buffer) {
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		}
		m_Stream->async_flush(yc);
	}
}

void HttpResponse::SendJsonBody(const Value& val, bool pretty)
{
	namespace http = boost::beast::http;

	set(http::field::content_type, "application/json");
	auto adapter = std::make_shared<decltype(BeastHttpMessageAdapter(*this))>(*this);
	JsonEncoder encoder(adapter, pretty);
	encoder.Encode(val);
	prepare_payload();
}

void HttpResponse::SendJsonError(int code, String info, String diagInfo, bool pretty, bool verbose)
{
	Dictionary::Ptr response = new Dictionary({ { "error", code } });

	if (!info.IsEmpty()) {
		response->Set("status", std::move(info));
	}

	if (verbose && !diagInfo.IsEmpty()) {
		response->Set("diagnostic_information", std::move(diagInfo));
	}

	result(code);

	SendJsonBody(response, pretty);
}

void HttpResponse::SendJsonError(const Dictionary::Ptr& params, int code, String info, String diagInfo)
{
	bool verbose = HttpUtility::GetLastParameter(params, "verbose");
	bool pretty = HttpUtility::GetLastParameter(params, "pretty");
	SendJsonError(code, std::move(info), std::move(diagInfo), pretty, verbose);
}
