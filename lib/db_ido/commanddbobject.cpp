// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	Command::Ptr command = static_pointer_cast<Command>(GetObject());

	return new Dictionary({
		{ "command_line", CompatUtility::GetCommandLine(command) }
	});
}

Dictionary::Ptr CommandDbObject::GetStatusFields() const
{
	return nullptr;
}
