/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "db_ido/commanddbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/command.hpp"
#include "icinga/compatutility.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_DBTYPE(Command, "command", DbObjectTypeCommand, "object_id", CommandDbObject);

CommandDbObject::CommandDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr CommandDbObject::GetConfigFields() const
{
	Dictionary::Ptr fields = new Dictionary();
	Command::Ptr command = static_pointer_cast<Command>(GetObject());

	fields->Set("command_line", CompatUtility::GetCommandLine(command));

	return fields;
}

Dictionary::Ptr CommandDbObject::GetStatusFields() const
{
	return nullptr;
}
