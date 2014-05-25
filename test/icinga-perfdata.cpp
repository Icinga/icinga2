/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "icinga/perfdatavalue.hpp"
#include "icinga/pluginutility.hpp"
#include <boost/test/unit_test.hpp>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(icinga_perfdata)

BOOST_AUTO_TEST_CASE(empty)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("");
	BOOST_CHECK(pd->GetLength() == 0);
}

BOOST_AUTO_TEST_CASE(simple)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("test=123456");
	BOOST_CHECK(pd->Get("test") == 123456);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK(str == "test=123456");
}

BOOST_AUTO_TEST_CASE(quotes)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("'hello world'=123456");
	BOOST_CHECK(pd->Get("hello world") == 123456);
}

BOOST_AUTO_TEST_CASE(multiple)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("testA=123456 testB=123456");
	BOOST_CHECK(pd->Get("testA") == 123456);
	BOOST_CHECK(pd->Get("testB") == 123456);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK(str == "testA=123456 testB=123456");
}

BOOST_AUTO_TEST_CASE(uom)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("test=123456B");

	PerfdataValue::Ptr pv = pd->Get("test");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 123456);
	BOOST_CHECK(!pv->GetCounter());
	BOOST_CHECK(pv->GetUnit() == "bytes");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK(str == "test=123456B");

	pd = PluginUtility::ParsePerfdata("test=1000ms;200;500");
	BOOST_CHECK(pd);

	pv = pd->Get("test");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "seconds");
	BOOST_CHECK(pv->GetWarn() == 0.2);
	BOOST_CHECK(pv->GetCrit() == 0.5);

	pd = PluginUtility::ParsePerfdata("test=1000ms");
	BOOST_CHECK(pd);

	pv = pd->Get("test");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 1);
	BOOST_CHECK(pv->GetUnit() == "seconds");
	BOOST_CHECK(pv->GetCrit() == Empty);
	BOOST_CHECK(pv->GetWarn() == Empty);
	BOOST_CHECK(pv->GetMin() == Empty);
	BOOST_CHECK(pv->GetMax() == Empty);

	str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK(str == "test=1s");
}

BOOST_AUTO_TEST_CASE(warncritminmax)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("test=123456B;1000;2000;3000;4000");

	PerfdataValue::Ptr pv = pd->Get("test");
	BOOST_CHECK(pv);

	BOOST_CHECK(pv->GetValue() == 123456);
	BOOST_CHECK(!pv->GetCounter());
	BOOST_CHECK(pv->GetUnit() == "bytes");
	BOOST_CHECK(pv->GetWarn() == 1000);
	BOOST_CHECK(pv->GetCrit() == 2000);
	BOOST_CHECK(pv->GetMin() == 3000);
	BOOST_CHECK(pv->GetMax() == 4000);

	String str = PluginUtility::FormatPerfdata(pd);
	BOOST_CHECK(str == "test=123456B;1000;2000;3000;4000");
}

BOOST_AUTO_TEST_CASE(invalid)
{
	BOOST_CHECK(PluginUtility::ParsePerfdata("test=1,23456") == "test=1,23456");
	BOOST_CHECK(PluginUtility::ParsePerfdata("test=123456;10%;20%") == "test=123456;10%;20%");
}

BOOST_AUTO_TEST_CASE(multi)
{
	Dictionary::Ptr pd = PluginUtility::ParsePerfdata("test::a=3 b=4");
	BOOST_CHECK(pd->Get("test::a") == 3);
	BOOST_CHECK(pd->Get("test::b") == 4);
}

BOOST_AUTO_TEST_SUITE_END()
