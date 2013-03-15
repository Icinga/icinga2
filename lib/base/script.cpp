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

#include "i2-base.h"

using namespace icinga;

REGISTER_TYPE(Script);

/**
 * Constructor for the Script class.
 *
 * @param serializedUpdate A serialized dictionary containing attributes.
 */
Script::Script(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("language", Attribute_Config, &m_Language);
	RegisterAttribute("code", Attribute_Config, &m_Code);
}

/**
 * @threadsafety Always.
 */
void Script::Start(void)
{
	assert(OwnsLock());

	SpawnInterpreter();
}

/**
 * @threadsafety Always.
 */
String Script::GetLanguage(void) const
{
	ObjectLock olock(this);

	return m_Language;
}

/**
 * @threadsafety Always.
 */
String Script::GetCode(void) const
{
	ObjectLock olock(this);

	return m_Code;
}

/**
 * @threadsafety Always.
 */
void Script::OnAttributeUpdate(const String& name)
{
	assert(!OwnsLock());

	if (name == "language" || name == "code")
		SpawnInterpreter();
}

/**
 * @threadsafety Always.
 */
void Script::SpawnInterpreter(void)
{
	Logger::Write(LogInformation, "base", "Reloading script '" + GetName() + "'");

	ScriptLanguage::Ptr language = ScriptLanguage::GetByName(GetLanguage());
	m_Interpreter = language->CreateInterpreter(GetSelf());
}
