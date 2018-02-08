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
#include "remote/apiuser.tcpp"
#include "base/configtype.hpp"
#include "base/base64.hpp"

using namespace icinga;

REGISTER_TYPE(ApiUser);

ApiUser::Ptr ApiUser::GetByClientCN(const String& cn)
{
	for (const ApiUser::Ptr& user : ConfigType::GetObjectsByType<ApiUser>()) {
		if (user->GetClientCN() == cn)
			return user;
	}

	return ApiUser::Ptr();
}

ApiUser::Ptr ApiUser::GetByAuthHeader(const String& auth_header) {
	String::SizeType pos = auth_header.FindFirstOf(" ");
	String username, password;

	if (pos != String::NPos && auth_header.SubStr(0, pos) == "Basic") {
		String credentials_base64 = auth_header.SubStr(pos + 1);
		String credentials = Base64::Decode(credentials_base64);

		String::SizeType cpos = credentials.FindFirstOf(":");

		if (cpos != String::NPos) {
			username = credentials.SubStr(0, cpos);
			password = credentials.SubStr(cpos + 1);
		}
	}

	const ApiUser::Ptr& user = ApiUser::GetByName(username);

	/* Deny authentication if 1) given password is empty 2) configured password does not match. */
	if (password.IsEmpty())
		return nullptr;
	else if (user && user->GetPassword() != password)
		return nullptr;

	return user;
}
