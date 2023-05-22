/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "perfdata/influxdbwriter-ti.hpp"

namespace icinga
{

/**
 * An Icinga InfluxDB v1 writer.
 *
 * @ingroup perfdata
 */
class InfluxdbWriter final : public ObjectImpl<InfluxdbWriter>
{
public:
	DECLARE_OBJECT(InfluxdbWriter);
	DECLARE_OBJECTNAME(InfluxdbWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	boost::beast::http::request<boost::beast::http::string_body> AssembleRequest(String body) override;
	Url::Ptr AssembleUrl() override;
};

}
