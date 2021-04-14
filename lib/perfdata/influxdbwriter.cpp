/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "perfdata/influxdbwriter.hpp"
#include "perfdata/influxdbwriter-ti.cpp"
#include "base/base64.hpp"
#include "remote/url.hpp"
#include "base/configtype.hpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <utility>

using namespace icinga;

REGISTER_TYPE(InfluxdbWriter);

REGISTER_STATSFUNCTION(InfluxdbWriter, &InfluxdbWriter::StatsFunc);

void InfluxdbWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	InfluxdbCommonWriter::StatsFunc<InfluxdbWriter>(status, perfdata);
}

boost::beast::http::request<boost::beast::http::string_body> InfluxdbWriter::AssembleRequest(String body)
{
	auto request (AssembleBaseRequest(std::move(body)));
	Dictionary::Ptr basicAuth = GetBasicAuth();

	if (basicAuth) {
		request.set(
			boost::beast::http::field::authorization,
			"Basic " + Base64::Encode(basicAuth->Get("username") + ":" + basicAuth->Get("password"))
		);
	}

	return std::move(request);
}

Url::Ptr InfluxdbWriter::AssembleUrl()
{
	auto url (AssembleBaseUrl());

	std::vector<String> path;
	path.emplace_back("write");
	url->SetPath(path);

	url->AddQueryElement("db", GetDatabase());

	if (!GetUsername().IsEmpty())
		url->AddQueryElement("u", GetUsername());
	if (!GetPassword().IsEmpty())
		url->AddQueryElement("p", GetPassword());

	return std::move(url);
}
