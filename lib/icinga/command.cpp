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

#include "icinga/command.h"

using namespace icinga;

Value Command::GetCommandLine(void) const
{
	return m_CommandLine;
}

double Command::GetTimeout(void) const
{
	if (m_Timeout.IsEmpty())
		return 300;
	else
		return m_Timeout;
}

Dictionary::Ptr Command::GetMacros(void) const
{
	return m_Macros;
}

Array::Ptr Command::GetExportMacros(void) const
{
	return m_ExportMacros;
}

Array::Ptr Command::GetEscapeMacros(void) const
{
	return m_EscapeMacros;
}

bool Command::ResolveMacro(const String& macro, const Dictionary::Ptr&, String *result) const
{
	Dictionary::Ptr macros = GetMacros();

	if (macros && macros->Contains(macro)) {
		*result = macros->Get(macro);
		return true;
	}

	return false;
}

void Command::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("command", m_CommandLine);
		bag->Set("timeout", m_Timeout);
		bag->Set("macros", m_Macros);
		bag->Set("export_macros", m_ExportMacros);
		bag->Set("escape_macros", m_EscapeMacros);
	}
}

void Command::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_CommandLine = bag->Get("command");
		m_Timeout = bag->Get("timeout");
		m_Macros = bag->Get("macros");
		m_Macros = bag->Get("macros");
		m_ExportMacros = bag->Get("export_macros");
		m_EscapeMacros = bag->Get("escape_macros");
	}
}
