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

#include "remote/apiaction.hpp"
#include "base/singleton.hpp"

using namespace icinga;

ApiAction::ApiAction(std::vector<String> types, Callback action)
	: m_Types(std::move(types)), m_Callback(std::move(action))
{ }

Value ApiAction::Invoke(const ConfigObject::Ptr& target, const Dictionary::Ptr& params)
{
	return m_Callback(target, params);
}

const std::vector<String>& ApiAction::GetTypes() const
{
	return m_Types;
}

ApiAction::Ptr ApiAction::GetByName(const String& name)
{
	return ApiActionRegistry::GetInstance()->GetItem(name);
}

void ApiAction::Register(const String& name, const ApiAction::Ptr& action)
{
	ApiActionRegistry::GetInstance()->Register(name, action);
}

void ApiAction::Unregister(const String& name)
{
	ApiActionRegistry::GetInstance()->Unregister(name);
}

ApiActionRegistry *ApiActionRegistry::GetInstance()
{
	return Singleton<ApiActionRegistry>::GetInstance();
}
