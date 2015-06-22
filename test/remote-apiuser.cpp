/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "remote/apiuser.hpp"
#include <boost/test/unit_test.hpp>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(remote_apiuser)

BOOST_AUTO_TEST_CASE(construct)
{
	ApiUser::Ptr apiuser = new ApiUser();
	BOOST_CHECK(apiuser);
}

BOOST_AUTO_TEST_CASE(get_password)
{
	ApiUser::Ptr apiuser = new ApiUser();
	apiuser->SetPassword("icingar0xx");

	BOOST_CHECK(apiuser->GetPassword() == "*****");
}

BOOST_AUTO_TEST_CASE(check_password)
{
	ApiUser::Ptr apiuser = new ApiUser();
	apiuser->SetPassword("icingar0xx");

	BOOST_CHECK(apiuser->CheckPassword("1cing4r0xx") == false);
	BOOST_CHECK(apiuser->CheckPassword("icingar0xx") == true);
}

BOOST_AUTO_TEST_SUITE_END()
