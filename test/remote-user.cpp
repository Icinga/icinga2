/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "base/tlsutility.hpp"
#include <BoostTestTargetConfig.h>

#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(api_user)

BOOST_AUTO_TEST_CASE(password)
{
#ifndef I2_DEBUG
	std::cout << "Only enabled in Debug builds..." << std::endl;
#else
	ApiUser::Ptr user = new ApiUser();
	String passwd = RandomString(16);
	String salt = RandomString(8);
	user->SetPassword("ThisShouldBeIgnored");
	user->SetPasswordHash(ApiUser::CreateHashedPasswordString(passwd, salt, true));

	BOOST_CHECK(user->GetPasswordHash() != passwd);

	Dictionary::Ptr passwdd = user->GetPasswordDict();

	BOOST_CHECK(passwdd);
	BOOST_CHECK(passwdd->Get("salt") == salt);
	BOOST_CHECK(user->ComparePassword(passwd));
	BOOST_CHECK(!user->ComparePassword("wrong password uwu!"));
#endif
}

BOOST_AUTO_TEST_SUITE_END()
