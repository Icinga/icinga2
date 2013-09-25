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

#include "db_ido/commanddbobject.h"
#include "db_ido/dbtype.h"
#include "db_ido/dbvalue.h"
#include "icinga/command.h"
#include "icinga/compatutility.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(CheckCommand, "command", DbObjectTypeCommand, "object_id", CommandDbObject);
REGISTER_DBTYPE(EventCommand, "command", DbObjectTypeCommand, "object_id", CommandDbObject);
REGISTER_DBTYPE(NotificationCommand, "command", DbObjectTypeCommand, "object_id", CommandDbObject);

CommandDbObject::CommandDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr CommandDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	Command::Ptr command = static_pointer_cast<Command>(GetObject());

	Dictionary::Ptr attrs;

	attrs = CompatUtility::GetCommandConfigAttributes(command);

	fields->Set("command_line", attrs->Get("command_line"));

	return fields;
}

Dictionary::Ptr CommandDbObject::GetStatusFields(void) const
{
	return Empty;
}
