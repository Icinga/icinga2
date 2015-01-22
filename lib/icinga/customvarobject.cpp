/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/customvarobject.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_TYPE(CustomVarObject);

boost::signals2::signal<void (const CustomVarObject::Ptr&, const Dictionary::Ptr& vars, const MessageOrigin&)> CustomVarObject::OnVarsChanged;

Dictionary::Ptr CustomVarObject::GetVars(void) const
{
	if (GetOverrideVars())
		return GetOverrideVars();
	else
		return GetVarsRaw();
}

void CustomVarObject::SetVars(const Dictionary::Ptr& vars, const MessageOrigin& origin)
{
	SetOverrideVars(vars);

	OnVarsChanged(this, vars, origin);
}

int CustomVarObject::GetModifiedAttributes(void) const
{
	/* does nothing by default */
	return 0;
}

void CustomVarObject::SetModifiedAttributes(int, const MessageOrigin&)
{
	/* does nothing by default */
}

bool CustomVarObject::IsVarOverridden(const String& name) const
{
	Dictionary::Ptr vars_override = GetOverrideVars();

	if (!vars_override)
		return false;

	return vars_override->Contains(name);
}
