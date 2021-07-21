/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "perfdata/influxdb2writer.hpp"
#include "perfdata/influxdb2writer-ti.cpp"
#include "remote/url.hpp"
#include "base/configtype.hpp"
#include "base/perfdatavalue.hpp"
#include "base/statsfunction.hpp"
#include <utility>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

using namespace icinga;

REGISTER_TYPE(Influxdb2Writer);

REGISTER_STATSFUNCTION(Influxdb2Writer, &Influxdb2Writer::StatsFunc);

void Influxdb2Writer::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	InfluxdbCommonWriter::StatsFunc<Influxdb2Writer>(status, perfdata);
}

boost::beast::http::request<boost::beast::http::string_body> Influxdb2Writer::AssembleRequest(String body)
{
	auto request (AssembleBaseRequest(std::move(body)));

	request.set(boost::beast::http::field::authorization, "Token " + GetAuthToken());

	return std::move(request);
}

Url::Ptr Influxdb2Writer::AssembleUrl()
{
	auto url (AssembleBaseUrl());

	std::vector<String> path ({"api", "v2", "write"});
	url->SetPath(path);

	url->AddQueryElement("org", GetOrganization());
	url->AddQueryElement("bucket", GetBucket());

	return std::move(url);
}
