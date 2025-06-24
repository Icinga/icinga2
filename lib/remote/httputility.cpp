/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/httputility.hpp"
#include "remote/url.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include <map>
#include <string>
#include <vector>
#include <boost/beast/http.hpp>

using namespace icinga;

HttpRequest::HttpRequest(boost::beast::http::request<boost::beast::http::string_body>&& other)
	: base_type(std::forward<base_type>(other)){}

const ApiUser::Ptr& HttpRequest::User()
{
	if (!user) {
		FetchUser();
	}
	return user;
}

void HttpRequest::FetchUser()
{
	user = ApiUser::GetByAuthHeader(std::string(at(boost::beast::http::field::authorization)));
}

void HttpRequest::User(const ApiUser::Ptr& user)
{
	this->user = user;
}

const Url::Ptr& HttpRequest::Url()
{
	if (!url) {
		url = new icinga::Url(std::string(target()));
	}
	return url;
}

const Dictionary::Ptr& HttpRequest::Params() const
{
	return params;
}

void HttpRequest::DecodeParams()
{
	if (!body().empty()) {
		Log(LogDebug, "HttpUtility")
			<< "Request body: '" << body() << '\'';

		params = JsonDecode(body());
	}

	if (!params)
		params = new Dictionary();

	std::map<String, std::vector<String>> query;
	for (const auto& kv : Url()->GetQuery()) {
		query[kv.first].emplace_back(kv.second);
	}

	for (auto& kv : query) {
		params->Set(kv.first, Array::FromVector(kv.second));
	}
}

Value HttpRequest::GetLastParameter(const String& key) const
{
	return HttpUtility::GetLastParameter(Params(), key);
}

HttpResponse::HttpResponse()
	: m_WriteDone(IoEngine::Get().GetIoContext()),
	  m_WriteReady(IoEngine::Get().GetIoContext())
{
}

//TODO: Maybe we don't need this. Signal a complete header implicitly? How?
void HttpResponse::Begin()
{
	m_WriteReady.Set();
	body().more = true;
}

//TODO: This becomes useless when everything uses the proper interface
void HttpResponse::Done()
{
	m_WriteReady.Set();
	body().more = false;
}

void HttpResponse::Write(AsioTlsStream& stream, boost::asio::yield_context& yc)
{
	m_WriteReady.Wait(yc);

	if (has_content_length()) {
		body().more = false;
	} else {
		chunked(true);
	}

	boost::system::error_code ec;
	Serializer sr(*this);

	try {
		do {
			if (body_type::size(body())) {
				boost::beast::http::async_write(stream, sr, yc[ec]);
				if (ec && ec != boost::beast::http::error::need_buffer) {
					throw boost::system::system_error{ec};
				}
				stream.async_flush(yc);
				m_WriteDone.Set();
				m_WriteDone.Clear();
			} else {
				body().moreData.Wait(yc);
			}
		} while (!sr.is_done());
	} catch (const std::exception& ex) {
		m_WriteError = true;
		m_WriteDone.Set();
		body().buffer.clear();
	}
}

void HttpResponse::Wait(boost::asio::yield_context& yc)
{
	m_WriteDone.Wait(yc);
}

Value HttpUtility::GetLastParameter(const Dictionary::Ptr& params, const String& key)
{
	Value varr = params->Get(key);

	if (!varr.IsObjectType<Array>())
		return varr;

	Array::Ptr arr = varr;

	if (arr->GetLength() == 0)
		return Empty;
	else
		return arr->Get(arr->GetLength() - 1);
}

void HttpResponse::SendJsonBody(const Dictionary::Ptr& params, const Value& val)
{
	namespace http = boost::beast::http;

	set(http::field::content_type, "application/json");
	body() << JsonEncode(val, params && HttpUtility::GetLastParameter(params, "pretty"));
	prepare_payload();
}

void HttpResponse::SendJsonError(const Dictionary::Ptr& params, int code, const String& info, const String& diagnosticInformation)
{
	Dictionary::Ptr body = new Dictionary({ { "error", code } });

	if (!info.IsEmpty()) {
		body->Set("status", info);
	}

	if (HttpUtility::GetLastParameter(params, "verbose") && !diagnosticInformation.IsEmpty()) {
		body->Set("diagnostic_information", diagnosticInformation);
	}

	result(code);

	SendJsonBody(HttpUtility::GetLastParameter(params, "pretty"), body);
}
