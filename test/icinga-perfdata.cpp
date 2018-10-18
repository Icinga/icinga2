/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
}

BOOST_AUTO_TEST_CASE(multi)
{
	Array::Ptr pd = PluginUtility::SplitPerfdata("test::a=3 b=4");
	BOOST_CHECK(pd->Get(0) == "test::a=3");
	BOOST_CHECK(pd->Get(1) == "test::b=4");
}

BOOST_AUTO_TEST_SUITE_END()
