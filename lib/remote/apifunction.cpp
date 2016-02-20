/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "remote/apifunction.hpp"
#include "base/singleton.hpp"

using namespace icinga;

ApiFunction::ApiFunction(const Callback& function)
: m_Callback(function)
{ }

Value ApiFunction::Invoke(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& arguments)
{
	return m_Callback(origin, arguments);
}

ApiFunction::Ptr ApiFunction::GetByName(const String& name)
{
	return ApiFunctionRegistry::GetInstance()->GetItem(name);
}

void ApiFunction::Register(const String& name, const ApiFunction::Ptr& function)
{
	ApiFunctionRegistry::GetInstance()->Register(name, function);
}

void ApiFunction::Unregister(const String& name)
{
	ApiFunctionRegistry::GetInstance()->Unregister(name);
}

ApiFunctionRegistry *ApiFunctionRegistry::GetInstance(void)
{
	return Singleton<ApiFunctionRegistry>::GetInstance();
}
