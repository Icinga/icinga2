/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/perfdatavalue.hpp"
#include "icinga/pluginutility.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_perfdata)

BOOST_AUTO_TEST_CASE(empty)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("");
	BOOST_CHECK_EQUAL(pd->GetLength(), 0);
}

BOOST_AUTO_TEST_CASE(simple)
{
	PerfdataValue::Ptr pdv = PerfdataValue::Parse("test=123456");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 123456);

	String str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=123456");
}

BOOST_AUTO_TEST_CASE(quotes)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("'hello world'=123456");
	BOOST_CHECK_EQUAL(pd->GetLength(), 1);

	PerfdataValue::Ptr pdv = PerfdataValue::Parse("'hello world'=123456");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "hello world");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 123456);
}

BOOST_AUTO_TEST_CASE(multiple)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("testA=123456 testB=123456");
	BOOST_CHECK_EQUAL(pd->GetLength(), 2);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK_EQUAL(str, "testA=123456 testB=123456");
}

BOOST_AUTO_TEST_CASE(multiline)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata(" 'testA'=123456  'testB'=123456");
	BOOST_CHECK_EQUAL(pd->GetLength(), 2);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK_EQUAL(str, "testA=123456 testB=123456");

	pd = PluginUtility::SplitPerfdata(" 'testA'=123456  \n'testB'=123456");
	BOOST_CHECK_EQUAL(pd->GetLength(), 2);

	str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK_EQUAL(str, "testA=123456 testB=123456");
}

BOOST_AUTO_TEST_CASE(normalize)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("testA=2m;3;4;1;5 testB=2foobar");
	BOOST_CHECK_EQUAL(pd->GetLength(), 2);

	String str = PluginUtility::FormatPerfdata(pd, true);
	BOOST_CHECK_EQUAL(str, "testA=120s;180;240;60;300 testB=2");
}

BOOST_AUTO_TEST_CASE(uom)
{
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=123456B");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 123456);
	BOOST_CHECK(!pv->GetCounter());
	BOOST_CHECK_EQUAL(pv->GetUnit(), "bytes");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	String str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=123456B");

	pv = PerfdataValue::Parse("test=1000ms;200;500");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "seconds");
	BOOST_CHECK_EQUAL(pv->GetWarn(), 0.2);
	BOOST_CHECK_EQUAL(pv->GetCrit(), 0.5);

	pv = PerfdataValue::Parse("test=1000ms");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "seconds");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1s");

	pv = PerfdataValue::Parse("test=1kAm");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 60 * 1000);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "ampere-seconds");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=60000As");

	pv = PerfdataValue::Parse("test=1MA");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1000 * 1000);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "amperes");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1000000A");

	pv = PerfdataValue::Parse("test=1gib");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1024 * 1024 * 1024);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "bits");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1073741824b");

	pv = PerfdataValue::Parse("test=1dBm");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "decibel-milliwatts");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1dBm");

	pv = PerfdataValue::Parse("test=1C");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "degrees-celsius");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1C");

	pv = PerfdataValue::Parse("test=1F");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "degrees-fahrenheit");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1F");

	pv = PerfdataValue::Parse("test=1K");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "degrees-kelvin");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1K");

	pv = PerfdataValue::Parse("test=1t");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1000 * 1000);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "grams");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1000000g");

	pv = PerfdataValue::Parse("test=1hl");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 100);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "liters");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=100l");

	pv = PerfdataValue::Parse("test=1lm");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "lumens");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1lm");

	pv = PerfdataValue::Parse("test=1TO");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1000.0 * 1000 * 1000 * 1000);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "ohms");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1000000000000O");

	pv = PerfdataValue::Parse("test=1PV");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1000.0 * 1000 * 1000 * 1000 * 1000);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "volts");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1000000000000000V");

	pv = PerfdataValue::Parse("test=1EWh");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1000.0 * 1000 * 1000 * 1000 * 1000 * 1000);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "watt-hours");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1000000000000000000Wh");

	pv = PerfdataValue::Parse("test=1000mW");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 1);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "watts");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=1W");

	pv = PerfdataValue::Parse("test=42c");
	BOOST_CHECK(pv);
	BOOST_CHECK_EQUAL(pv->GetValue(), 42);
	BOOST_CHECK(pv->GetCounter());
	BOOST_CHECK_EQUAL(pv->GetUnit(), "");
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMax(), Empty);

	str = pv->Format();
	BOOST_CHECK_EQUAL(str, "test=42c");
}

BOOST_AUTO_TEST_CASE(warncritminmax)
{
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=123456B;1000;2000;3000;4000");
	BOOST_CHECK(pv);

	BOOST_CHECK_EQUAL(pv->GetValue(), 123456);
	BOOST_CHECK(!pv->GetCounter());
	BOOST_CHECK_EQUAL(pv->GetUnit(), "bytes");
	BOOST_CHECK_EQUAL(pv->GetWarn(), 1000);
	BOOST_CHECK_EQUAL(pv->GetCrit(), 2000);
	BOOST_CHECK_EQUAL(pv->GetMin(), 3000);
	BOOST_CHECK_EQUAL(pv->GetMax(), 4000);

	BOOST_CHECK_EQUAL(pv->Format(), "test=123456B;1000;2000;3000;4000");
}

BOOST_AUTO_TEST_CASE(ignore_warn_crit_ranges)
{
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=123456;1000:2000;0:3000;3000;4000");
	BOOST_CHECK(pv);
	BOOST_CHECK_EQUAL(pv->GetValue(), 123456);
	BOOST_CHECK_EQUAL(pv->GetWarn(), Empty);
	BOOST_CHECK_EQUAL(pv->GetCrit(), Empty);
	BOOST_CHECK_EQUAL(pv->GetMin(), 3000);
	BOOST_CHECK_EQUAL(pv->GetMax(), 4000);

	BOOST_CHECK_EQUAL(pv->Format(), "test=123456;;;3000;4000");
}

BOOST_AUTO_TEST_CASE(empty_warn_crit_min_max)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("testA=5;;7;1;9 testB=5;7;;1;9 testC=5;;;1;9 testD=2m;;;1 testE=5;;7;;");
	BOOST_CHECK_EQUAL(pd->GetLength(), 5);

	String str = PluginUtility::FormatPerfdata(pd, true);
	BOOST_CHECK_EQUAL(str, "testA=5;;7;1;9 testB=5;7;;1;9 testC=5;;;1;9 testD=120s;;;60 testE=5;;7");
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
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=inf"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=Inf"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=-inf"), boost::exception);
	BOOST_CHECK_THROW(PerfdataValue::Parse("test=-Inf"), boost::exception);
}

BOOST_AUTO_TEST_CASE(multi)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("test::a=3 b=4");
	BOOST_CHECK_EQUAL(pd->Get(0), "test::a=3");
	BOOST_CHECK_EQUAL(pd->Get(1), "test::b=4");
}

BOOST_AUTO_TEST_CASE(scientificnotation)
{
	PerfdataValue::Ptr pdv = PerfdataValue::Parse("test=1.1e+1");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 11);

	String str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=11");

	pdv = PerfdataValue::Parse("test=1.1e1");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 11);

	str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=11");

	pdv = PerfdataValue::Parse("test=1.1e-1");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 0.11);

	str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=0.110000");

	pdv = PerfdataValue::Parse("test=1.1E1");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 11);

	str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=11");

	pdv = PerfdataValue::Parse("test=1.1E-1");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 0.11);

	str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=0.110000");

	pdv = PerfdataValue::Parse("test=1.1E-1;1.2e+1;1.3E-1;1.4e-2;1.5E2");
	BOOST_CHECK_EQUAL(pdv->GetLabel(), "test");
	BOOST_CHECK_EQUAL(pdv->GetValue(), 0.11);
	BOOST_CHECK_EQUAL(pdv->GetWarn(), 12);
	BOOST_CHECK_EQUAL(pdv->GetCrit(), 0.13);
	BOOST_CHECK_EQUAL(pdv->GetMin(), 0.014);
	BOOST_CHECK_EQUAL(pdv->GetMax(), 150);

	str = pdv->Format();
	BOOST_CHECK_EQUAL(str, "test=0.110000;12;0.130000;0.014000;150");
}

BOOST_AUTO_TEST_CASE(parse_edgecases)
{
	// Trailing decimal point
	PerfdataValue::Ptr pv = PerfdataValue::Parse("test=23.");
	BOOST_CHECK(pv);
	BOOST_CHECK_EQUAL(pv->GetValue(), 23.0);

	// Leading decimal point
	pv = PerfdataValue::Parse("test=.42");
	BOOST_CHECK(pv);
	BOOST_CHECK_EQUAL(pv->GetValue(), 0.42);

	// E both as exponent and unit prefix
	pv = PerfdataValue::Parse("test=+1.5E-15EB");
	BOOST_CHECK(pv);
	BOOST_CHECK_EQUAL(pv->GetValue(), 1.5e3);
	BOOST_CHECK_EQUAL(pv->GetUnit(), "bytes");
}

BOOST_AUTO_TEST_SUITE_END()
