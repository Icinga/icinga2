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

#include "base/utility.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_match)

BOOST_AUTO_TEST_CASE(tolong)
{
	BOOST_CHECK(Utility::Match("*", "hello"));
	BOOST_CHECK(!Utility::Match("\\**", "hello"));
	BOOST_CHECK(Utility::Match("\\**", "*ello"));
	BOOST_CHECK(Utility::Match("?e*l?", "hello"));
	BOOST_CHECK(Utility::Match("?e*l?", "helo"));
	BOOST_CHECK(!Utility::Match("world", "hello"));
	BOOST_CHECK(!Utility::Match("hee*", "hello"));
	BOOST_CHECK(Utility::Match("he??o", "hello"));
	BOOST_CHECK(Utility::Match("he?", "hel"));
	BOOST_CHECK(Utility::Match("he*", "hello"));
	BOOST_CHECK(Utility::Match("he*o", "heo"));
	BOOST_CHECK(Utility::Match("he**o", "heo"));
	BOOST_CHECK(Utility::Match("he**o", "hello"));
}

BOOST_AUTO_TEST_SUITE_END()
