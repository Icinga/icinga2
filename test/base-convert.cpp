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

	String str = "hello";
	BOOST_CHECK(Convert::ToString(str) == "hello");

	BOOST_CHECK(Convert::ToString(Value(7)) == "7");
	BOOST_CHECK(Convert::ToString(Value(7.5)) == "7.500000");
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
