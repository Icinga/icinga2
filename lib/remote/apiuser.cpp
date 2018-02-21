/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "remote/apiuser-ti.cpp"
#include "base/configtype.hpp"
#include "base/base64.hpp"
#include "base/tlsutility.hpp"

using namespace icinga;

REGISTER_TYPE(ApiUser);

void ApiUser::OnConfigLoaded(void)
{
	ObjectImpl<ApiUser>::OnConfigLoaded();

	if (GetPasswordHash().IsEmpty()) {
		String hashedPassword =	CreateHashedPasswordString(GetPassword(), RandomString(8), 5);
		VERIFY(hashedPassword != String());
		SetPasswordHash(hashedPassword);
		SetPassword("********");
	}
}

ApiUser::Ptr ApiUser::GetByClientCN(const String& cn)
{
	for (const ApiUser::Ptr& user : ConfigType::GetObjectsByType<ApiUser>()) {
		if (user->GetClientCN() == cn)
			return user;
	}

	return nullptr;
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

	/* Deny authentication if:
	 * 1) user does not exist
	 * 2) given password is empty
	 * 2) configured password does not match.
	 */
	if (!user || password.IsEmpty())
		return nullptr;
	else if (user && user->GetPassword() != password) {
		Dictionary::Ptr passwordDict = user->GetPasswordDict();
		if (!passwordDict || !ComparePassword(passwordDict->Get("password"), password, passwordDict->Get("salt")))
			return nullptr;
	}

	return user;
}

Dictionary::Ptr ApiUser::GetPasswordDict(void) const
{
	String password = GetPasswordHash();
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
