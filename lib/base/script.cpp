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

REGISTER_TYPE(Script, NULL);

/**
 * Constructor for the Script class.
 *
 * @param properties A serialized dictionary containing attributes.
 */
Script::Script(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{ }

void Script::OnRegistrationCompleted(void)
{
	DynamicObject::OnRegistrationCompleted();

	SpawnInterpreter();
}

String Script::GetLanguage(void) const
{
	return Get("language");
}

String Script::GetCode(void) const
{
	return Get("code");
}

void Script::OnAttributeUpdate(const String& name, const Value& oldValue)
{
	if (name == "language" || name == "code")
		SpawnInterpreter();
}

void Script::SpawnInterpreter(void)
{
	Logger::Write(LogInformation, "base", "Reloading script '" + GetName() + "'");

	ScriptLanguage::Ptr language = ScriptLanguage::GetByName(GetLanguage());
	m_Interpreter = language->CreateInterpreter(GetSelf());
}
