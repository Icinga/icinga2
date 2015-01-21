/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/command.hpp"
#include "base/function.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_TYPE(Command);
REGISTER_SCRIPTFUNCTION(ValidateCommandAttributes, &Command::ValidateAttributes);

int Command::GetModifiedAttributes(void) const
{
	int attrs = 0;

	if (GetOverrideVars())
		attrs |= ModAttrCustomVariable;

	return attrs;
}

void Command::SetModifiedAttributes(int flags, const MessageOrigin& origin)
{
	if ((flags & ModAttrCustomVariable) == 0) {
		SetOverrideVars(Empty);
		OnVarsChanged(this, GetVars(), origin);
	}
}

void Command::ValidateAttributes(const String& location, const Command::Ptr& object)
{
	if (object->GetArguments() != Empty && !object->GetCommandLine().IsObjectType<Array>()) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": Attribute 'command' must be an array if the 'arguments' attribute is set.", object->GetDebugInfo()));
	}
}

