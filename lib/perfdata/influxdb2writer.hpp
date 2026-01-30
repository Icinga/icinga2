// SPDX-FileCopyrightText: 2021 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INFLUXDB2WRITER_H
#define INFLUXDB2WRITER_H

#include "perfdata/influxdb2writer-ti.hpp"
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

namespace icinga
{

/**
 * An Icinga InfluxDB v2 writer.
 *
 * @ingroup perfdata
 */
class Influxdb2Writer final : public ObjectImpl<Influxdb2Writer>
{
public:
	DECLARE_OBJECT(Influxdb2Writer);
	DECLARE_OBJECTNAME(Influxdb2Writer);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

protected:
	boost::beast::http::request<boost::beast::http::string_body> AssembleRequest(String body) override;
	Url::Ptr AssembleUrl() override;
};

}

#endif /* INFLUXDB2WRITER_H */
