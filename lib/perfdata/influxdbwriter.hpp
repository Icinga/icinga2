// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INFLUXDBWRITER_H
#define INFLUXDBWRITER_H

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

#endif /* INFLUXDBWRITER_H */
