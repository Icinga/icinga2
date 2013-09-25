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

#ifndef COMMAND_H
#define COMMAND_H

#include "icinga/macroresolver.h"
#include "base/i2-base.h"
#include "base/array.h"
#include "base/dynamicobject.h"
#include "base/logger_fwd.h"
#include <set>

namespace icinga
{

/**
 * A command.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Command : public DynamicObject, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(Command);

	//virtual Dictionary::Ptr Execute(const Object::Ptr& context) = 0;

	Value GetCommandLine(void) const;
	double GetTimeout(void) const;

	Dictionary::Ptr GetMacros(void) const;
	Array::Ptr GetExportMacros(void) const;
	Array::Ptr GetEscapeMacros(void) const;
	virtual bool ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const;

protected:
	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	Value m_CommandLine;
	Value m_Timeout;
	Dictionary::Ptr m_Macros;
	Array::Ptr m_ExportMacros;
	Array::Ptr m_EscapeMacros;
};

}

#endif /* COMMAND_H */
