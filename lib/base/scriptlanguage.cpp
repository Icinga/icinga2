/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/scriptlanguage.h"

using namespace icinga;

ScriptLanguage::ScriptLanguage(void)
{ }

void ScriptLanguage::Register(const String& name, const ScriptLanguage::Ptr& language)
{
	boost::mutex::scoped_lock lock(GetMutex());

	GetLanguages()[name] = language;
}

void ScriptLanguage::Unregister(const String& name)
{
	boost::mutex::scoped_lock lock(GetMutex());

	GetLanguages().erase(name);
}

ScriptLanguage::Ptr ScriptLanguage::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetMutex());

	std::map<String, ScriptLanguage::Ptr>::iterator it;

	it = GetLanguages().find(name);

	if (it == GetLanguages().end())
		return ScriptLanguage::Ptr();

	return it->second;
}

boost::mutex& ScriptLanguage::GetMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}

std::map<String, ScriptLanguage::Ptr>& ScriptLanguage::GetLanguages(void)
{
	static std::map<String, ScriptLanguage::Ptr> languages;
	return languages;
}
