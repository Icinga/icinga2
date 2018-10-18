/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef SCRIPTGLOBAL_H
#define SCRIPTGLOBAL_H

#include "base/i2-base.hpp"
#include "base/namespace.hpp"

namespace icinga
{

/**
 * Global script variables.
 *
 * @ingroup base
 */
class ScriptGlobal
{
public:
	static Value Get(const String& name, const Value *defaultValue = nullptr);
	static void Set(const String& name, const Value& value, bool overrideFrozen = false);
	static void SetConst(const String& name, const Value& value);
	static bool Exists(const String& name);

	static void WriteToFile(const String& filename);

	static Namespace::Ptr GetGlobals();

private:
	static Namespace::Ptr m_Globals;
};

}

#endif /* SCRIPTGLOBAL_H */
