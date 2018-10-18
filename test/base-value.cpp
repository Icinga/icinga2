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

#include "base/value.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_value)

BOOST_AUTO_TEST_CASE(scalar)
{
	Value v;

	v = 3;
	BOOST_CHECK(v.IsScalar());

	v = "hello";
	BOOST_CHECK(v.IsScalar());

	v = Empty;
	BOOST_CHECK(!v.IsScalar());
}

BOOST_AUTO_TEST_CASE(convert)
{
	Value v;
	BOOST_CHECK(v.IsEmpty());
	BOOST_CHECK(v == "");
	BOOST_CHECK(static_cast<double>(v) == 0);
	BOOST_CHECK(!v.IsScalar());
	BOOST_CHECK(!v.IsObjectType<Object>());

	BOOST_CHECK(v + "hello" == "hello");
	BOOST_CHECK("hello" + v == "hello");
}

BOOST_AUTO_TEST_CASE(format)
{
	Value v = 3;

	std::ostringstream obuf;
	obuf << v;

	BOOST_CHECK(obuf.str() == "3");

	std::istringstream ibuf("3");
	ibuf >> v;

	BOOST_CHECK(v != 3);
}

BOOST_AUTO_TEST_SUITE_END()
