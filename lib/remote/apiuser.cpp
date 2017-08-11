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
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_TYPE(ApiUser);

void ApiUser::OnConfigLoaded(void)
{
	ObjectImpl<ApiUser>::OnConfigLoaded();

	if (this->GetPasswordHash().IsEmpty())
		SetPasswordHash(CreateHashedPasswordString(GetPassword(), RandomString(8), true));
}

ApiUser::Ptr ApiUser::GetByClientCN(const String& cn)
{
	for (const ApiUser::Ptr& user : ConfigType::GetObjectsByType<ApiUser>()) {
		if (user->GetClientCN() == cn)
			return user;
	}

	return ApiUser::Ptr();
}

ApiUser::Ptr ApiUser::GetByAuthHeader(const String& auth_header)
{
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

bool ApiUser::ComparePassword(String password) const
{
	Dictionary::Ptr passwordDict = this->GetPasswordDict();
	String thisPassword = passwordDict->Get("password");
	String otherPassword = CreateHashedPasswordString(password, passwordDict->Get("salt"), false);

	const char *p1 = otherPassword.CStr();
	const char *p2 = thisPassword.CStr();

	volatile char c = 0;

	for (size_t i=0; i<64; ++i)
		c |= p1[i] ^ p2[i];

	return (c == 0);
}

Dictionary::Ptr ApiUser::GetPasswordDict(void) const
{
	String password = this->GetPasswordHash();
	if (password.IsEmpty() || password[0] != '$')
		return nullptr;

	String::SizeType saltBegin = password.FindFirstOf('$', 1);
	String::SizeType passwordBegin = password.FindFirstOf('$', saltBegin+1);

	if (saltBegin == String::NPos || saltBegin == 1 || passwordBegin == String::NPos)
		return nullptr;

	Dictionary::Ptr passwordDict = new Dictionary();
	passwordDict->Set("algorithm", password.SubStr(1, saltBegin - 1));
	passwordDict->Set("salt", password.SubStr(saltBegin + 1, passwordBegin - saltBegin - 1));
	passwordDict->Set("password", password.SubStr(passwordBegin + 1));

	return passwordDict;
}

String ApiUser::CreateHashedPasswordString(const String& password, const String& salt, const bool shadow)
{
	if (shadow)
		//Using /etc/shadow password format. The 5 means SHA256 is being used
		return String("$5$" + salt + "$" + PBKDF2_SHA256(password, salt, 1000));
	else
		return PBKDF2_SHA256(password, salt, 1000);

}
