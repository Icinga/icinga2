/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/convert.h"
#include "base/object.h"
#include <boost/test/unit_test.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_convert)

BOOST_AUTO_TEST_CASE(tolong)
{
	BOOST_CHECK_THROW(Convert::ToLong(" 7"), boost::exception);
	BOOST_CHECK(Convert::ToLong("-7") == -7);
	BOOST_CHECK_THROW(Convert::ToLong("7a"), boost::exception);
}

BOOST_AUTO_TEST_CASE(todouble)
{
	BOOST_CHECK_THROW(Convert::ToDouble(" 7.3"), boost::exception);
	BOOST_CHECK(Convert::ToDouble("-7.3") == -7.3);
	BOOST_CHECK_THROW(Convert::ToDouble("7.3a"), boost::exception);
}

BOOST_AUTO_TEST_CASE(tostring)
{
	BOOST_CHECK(Convert::ToString(7) == "7");
	BOOST_CHECK(Convert::ToString(7.3) == "7.3");
	BOOST_CHECK(Convert::ToString("hello") == "hello");

	Object::Ptr object = boost::make_shared<Object>();
	BOOST_CHECK(Convert::ToString(object) == "Object of type 'icinga::Object'");
}

BOOST_AUTO_TEST_CASE(tobool)
{
	BOOST_CHECK_THROW(Convert::ToBool("a"), boost::exception);
	BOOST_CHECK(Convert::ToBool("0") == false);
	BOOST_CHECK(Convert::ToBool("1") == true);
	BOOST_CHECK(Convert::ToBool("2") == true);
}

BOOST_AUTO_TEST_SUITE_END()
