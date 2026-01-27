// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/convert.hpp"
#include "base/object.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_convert)

BOOST_AUTO_TEST_CASE(tolong)
{
	BOOST_CHECK_THROW(Convert::ToLong(" 7"), boost::exception);
	BOOST_CHECK(Convert::ToLong("-7") == -7);
	BOOST_CHECK_THROW(Convert::ToLong("7a"), boost::exception);

	BOOST_CHECK(Convert::ToLong(Value(-7)) == -7);

	BOOST_CHECK(Convert::ToLong(3.141386593) == 3);
}

BOOST_AUTO_TEST_CASE(todouble)
{
	BOOST_CHECK_THROW(Convert::ToDouble(" 7.3"), boost::exception);
	BOOST_CHECK(Convert::ToDouble("-7.3") == -7.3);
	BOOST_CHECK_THROW(Convert::ToDouble("7.3a"), boost::exception);
	BOOST_CHECK(Convert::ToDouble(Value(-7.3)) == -7.3);
}

BOOST_AUTO_TEST_CASE(tostring)
{
	BOOST_CHECK(Convert::ToString(7) == "7");
	BOOST_CHECK(Convert::ToString(7.5) == "7.500000");
	BOOST_CHECK(Convert::ToString("hello") == "hello");
	BOOST_CHECK(Convert::ToString(18446744073709551616.0) == "18446744073709551616"); // pow(2, 64)

	String str = "hello";
	BOOST_CHECK(Convert::ToString(str) == "hello");

	BOOST_CHECK(Convert::ToString(Value(7)) == "7");
	BOOST_CHECK(Convert::ToString(Value(7.5)) == "7.500000");
	BOOST_CHECK(Convert::ToString(Value(18446744073709551616.0)) == "18446744073709551616"); // pow(2, 64)
	BOOST_CHECK(Convert::ToString(Value("hello")) == "hello");
	BOOST_CHECK(Convert::ToString(Value("hello hello")) == "hello hello");
}

BOOST_AUTO_TEST_CASE(tobool)
{
	BOOST_CHECK(Convert::ToBool("a") == true);
	BOOST_CHECK(Convert::ToBool("0") == true);
	BOOST_CHECK(Convert::ToBool("1") == true);
	BOOST_CHECK(Convert::ToBool("2") == true);
	BOOST_CHECK(Convert::ToBool(1) == true);
	BOOST_CHECK(Convert::ToBool(0) == false);
	BOOST_CHECK(Convert::ToBool(Value(true)) == true);
	BOOST_CHECK(Convert::ToBool(Value(false)) == false);
}

BOOST_AUTO_TEST_SUITE_END()
