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
