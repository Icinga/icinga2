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

	for (auto& [key, val] : query) {
		m_Params->Set(key, new Array{std::move(val)});
	}
}

Value HttpRequest::GetLastParameter(const String& key) const
{
	return HttpUtility::GetLastParameter(Params(), key);
}

bool HttpRequest::IsPretty() const
{
	return GetLastParameter("pretty");
}

bool HttpRequest::IsVerbose() const
{
	return GetLastParameter("verbose");
}

HttpResponse::HttpResponse(Shared<AsioTlsStream>::Ptr stream)
	: m_Stream(std::move(stream))
{
}

void HttpResponse::Write(boost::asio::yield_context yc)
{
	if (!chunked()) {
		ASSERT(!m_Serializer->is_header_done());
		prepare_payload();
	}
	
	boost::system::error_code ec;
	boost::beast::http::async_write(*m_Stream, *m_Serializer, yc[ec]);
	if (ec && ec != boost::beast::http::error::need_buffer) {
		if (!yc.ec_) {
			BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
		} else {
			*yc.ec_ = ec;
			return;
		}
	}
	m_Stream->async_flush(yc);

	ASSERT(chunked() || m_Serializer->is_done());
}

void HttpResponse::Reset()
{
	m_Serializer.reset(new Serializer{*this});
	content_length(boost::none);
}

void HttpResponse::StartStreaming()
{
	ASSERT(body().Size() == 0 && !m_Serializer->is_header_done());
	body().Begin();
	chunked(true);
}

void HttpResponse::SendJsonBody(const Value& val, bool pretty)
{
	namespace http = boost::beast::http;

	set(http::field::content_type, "application/json");
	auto adapter = std::make_shared<decltype(BeastHttpMessageAdapter(*this))>(*this);
	JsonEncoder encoder(adapter, pretty);
	encoder.Encode(val);
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
	bool verbose = params && HttpUtility::GetLastParameter(params, "verbose");
	bool pretty = params && HttpUtility::GetLastParameter(params, "pretty");
	SendJsonError(code, std::move(info), std::move(diagInfo), pretty, verbose);
}
