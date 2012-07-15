/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-cib.h"

using namespace icinga;

string ConfigObjectAdapter::GetType(void) const
{
	return m_ConfigObject->GetType();
}

string ConfigObjectAdapter::GetName(void) const
{
	return m_ConfigObject->GetName();
}

bool ConfigObjectAdapter::IsLocal(void) const
{
	return m_ConfigObject->IsLocal();
}

ConfigObject::Ptr ConfigObjectAdapter::GetConfigObject() const
{
	return m_ConfigObject;
}

void ConfigObjectAdapter::RemoveTag(const string& key)
{
	m_ConfigObject->RemoveTag(key);
}

ScriptTask::Ptr ConfigObjectAdapter::InvokeHook(const string& hook,
	const vector<Variant>& arguments, ScriptTask::CompletionCallback callback)
{
	return m_ConfigObject->InvokeHook(hook, arguments, callback);
}