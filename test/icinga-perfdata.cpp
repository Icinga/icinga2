/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/perfdatavalue.hpp"
#include "icinga/pluginutility.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_perfdata)

BOOST_AUTO_TEST_CASE(empty)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("");
	BOOST_CHECK(pd->GetLength() == 0);
}

BOOST_AUTO_TEST_CASE(simple)
{
	PerfdataValue::Ptr pdv = PerfdataValue::Parse("test=123456");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 123456);

	String str = pdv->Format();
	BOOST_CHECK(str == "test=123456");
}

BOOST_AUTO_TEST_CASE(quotes)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("'hello world'=123456");
	BOOST_CHECK(pd->GetLength() == 1);
	
	PerfdataValue::Ptr pdv = PerfdataValue::Parse("'hello world'=123456");
	BOOST_CHECK(pdv->GetLabel() == "hello world");
	BOOST_CHECK(pdv->GetValue() == 123456);
}

BOOST_AUTO_TEST_CASE(multiple)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("testA=123456 testB=123456");
	BOOST_CHECK(pd->GetLength() == 2);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK(str == "testA=123456 testB=123456");
}

BOOST_AUTO_TEST_CASE(normalize)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("testA=2m;3;4;1;5 testB=2foobar");
	BOOST_CHECK(pd->GetLength() == 2);

	String str = PluginUtility::FormatPerfdata(pd, true);
	BOOST_CHECK(str == "testA=120s;180;240;60;300 testB=2");
}

BOOST_AUTO_TEST_CASE(uom)
{
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=123456B");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 123456);
	BOOST_CHECK(!pv->GetCounter());
	BOOST_CHECK(pv->GetUnit() == "bytes");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	String str = pv->Format();
	BOOST_CHECK(str == "test=123456B");

	pv = PerfdataValue::Parse("test=1000ms;200;500");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "seconds");
	BOOST_CHECK(pv->GetWarn() == 0.2);
	BOOST_CHECK(pv->GetCrit() == 0.5);

	pv = PerfdataValue::Parse("test=1000ms");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "seconds");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1s");

	pv = PerfdataValue::Parse("test=1kAm");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 60 * 1000);
	BOOST_CHECK(pv->GetUnit() == "ampere-seconds");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=60000As");

	pv = PerfdataValue::Parse("test=1MA");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1000 * 1000);
	BOOST_CHECK(pv->GetUnit() == "amperes");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1000000A");

	pv = PerfdataValue::Parse("test=1gib");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1024 * 1024 * 1024);
	BOOST_CHECK(pv->GetUnit() == "bits");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1073741824b");

	pv = PerfdataValue::Parse("test=1dBm");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "decibel-milliwatts");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1dBm");

	pv = PerfdataValue::Parse("test=1C");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "degrees-celsius");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1C");

	pv = PerfdataValue::Parse("test=1F");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "degrees-fahrenheit");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1F");

	pv = PerfdataValue::Parse("test=1K");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "degrees-kelvin");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1K");

	pv = PerfdataValue::Parse("test=1t");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1000 * 1000);
	BOOST_CHECK(pv->GetUnit() == "grams");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1000000g");

	pv = PerfdataValue::Parse("test=1hl");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 100);
	BOOST_CHECK(pv->GetUnit() == "liters");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=100l");

	pv = PerfdataValue::Parse("test=1lm");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "lumens");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1lm");

	pv = PerfdataValue::Parse("test=1TO");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1000.0 * 1000 * 1000 * 1000);
	BOOST_CHECK(pv->GetUnit() == "ohms");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1000000000000O");

	pv = PerfdataValue::Parse("test=1PV");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1000.0 * 1000 * 1000 * 1000 * 1000);
	BOOST_CHECK(pv->GetUnit() == "volts");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1000000000000000V");

	pv = PerfdataValue::Parse("test=1EWh");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1000.0 * 1000 * 1000 * 1000 * 1000 * 1000);
	BOOST_CHECK(pv->GetUnit() == "watt-hours");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1000000000000000000Wh");

	pv = PerfdataValue::Parse("test=1000mW");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "watts");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = pv->Format();
	BOOST_CHECK(str == "test=1W");
}

BOOST_AUTO_TEST_CASE(warncritminmax)
{
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=123456B;1000;2000;3000;4000");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 123456);
	BOOST_CHECK(!pv->GetCounter());
	BOOST_CHECK(pv->GetUnit() == "bytes");
	BOOST_CHECK(pv->GetWarn() == 1000);
	BOOST_CHECK(pv->GetCrit() == 2000);
	BOOST_CHECK(pv->GetMin() == 3000);
	BOOST_CHECK(pv->GetMax() == 4000);

	BOOST_CHECK(pv->Format() == "test=123456B;1000;2000;3000;4000");
}

BOOST_AUTO_TEST_CASE(ignore_invalid_warn_crit_min_max)
{
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=123456;1000:2000;0:3000;3000;4000");
	BOOST_CHECK(pv);
	BOOST_CHECK(pv->GetValue() == 123456);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetMin() == 3000);
	BOOST_CHECK(pv->GetMax() == 4000);

	BOOST_CHECK(pv->Format() == "test=123456");
}

BOOST_AUTO_TEST_CASE(invalid)
{
	BOOST_CHECK_THROW(PerfdataValue::Parse("123456"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=1,23456"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=123_456"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test="), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=123,456;1;1;1;1"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=1;123,456;1;1;1"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=1;1;123,456;1;1"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=1;1;1;123,456;1"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=1;1;1;1;123,456"), boost::exception);
}

BOOST_AUTO_TEST_CASE(multi)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("test::a=3 b=4");
	BOOST_CHECK(pd->Get(0) == "test::a=3");
	BOOST_CHECK(pd->Get(1) == "test::b=4");
}

BOOST_AUTO_TEST_CASE(scientificnotation)
{
	PerfdataValue::Ptr pdv = PerfdataValue::Parse("test=1.1e+1");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 11);

	String str = pdv->Format();
	BOOST_CHECK(str == "test=11");

	pdv = PerfdataValue::Parse("test=1.1e1");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 11);

	str = pdv->Format();
	BOOST_CHECK(str == "test=11");

	pdv = PerfdataValue::Parse("test=1.1e-1");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 0.11);

	str = pdv->Format();
	BOOST_CHECK(str == "test=0.110000");

	pdv = PerfdataValue::Parse("test=1.1E1");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 11);

	str = pdv->Format();
	BOOST_CHECK(str == "test=11");

	pdv = PerfdataValue::Parse("test=1.1E-1");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 0.11);

	str = pdv->Format();
	BOOST_CHECK(str == "test=0.110000");

	pdv = PerfdataValue::Parse("test=1.1E-1;1.2e+1;1.3E-1;1.4e-2;1.5E2");
	BOOST_CHECK(pdv->GetLabel() == "test");
	BOOST_CHECK(pdv->GetValue() == 0.11);
	BOOST_CHECK(pdv->GetWarn() == 12);
	BOOST_CHECK(pdv->GetCrit() == 0.13);
	BOOST_CHECK(pdv->GetMin() == 0.014);
	BOOST_CHECK(pdv->GetMax() == 150);

	str = pdv->Format();
	BOOST_CHECK(str == "test=0.110000;12;0.130000;0.014000;150");
}

BOOST_AUTO_TEST_CASE(parse_edgecases)
{
	// Trailing decimal point
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=23.");
	BOOST_CHECK(pv);
	BOOST_CHECK(pv->GetValue() == 23.0);

	// Leading decimal point
	pv = PerfdataValue::Parse("test=.42");
	BOOST_CHECK(pv);
	BOOST_CHECK(pv->GetValue() == 0.42);

	// E both as exponent and unit prefix
	pv = PerfdataValue::Parse("test=+1.5E-15EB");
	BOOST_CHECK(pv);
	BOOST_CHECK(pv->GetValue() == 1.5e3);
	BOOST_CHECK(pv->GetUnit() == "bytes");
}

BOOST_AUTO_TEST_SUITE_END()
