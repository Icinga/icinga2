/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/perfdatavalue.hpp"
#include "icinga/pluginutility.hpp"
#include "lib/perfdata/influxdbwriter.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_influxdbwriter)

BOOST_AUTO_TEST_CASE(json_label)
{
	BOOST_CHECK(InfluxdbWriter::IXBuildMetric("not-a-json-label") == ",metric=not-a-json-label");
	BOOST_CHECK(InfluxdbWriter::IXBuildMetric("{\"metric\":\"test-metric\",\"label\":\"test-label\"}") == ",label=test-label,metric=test-metric");
	BOOST_CHECK(InfluxdbWriter::IXBuildMetric("{\"metric\":\"test-metric\",\"label\":\"test-label\",\"foo\":\"bar\"}") == ",foo=bar,label=test-label,metric=test-metric");
}

BOOST_AUTO_TEST_SUITE_END()
