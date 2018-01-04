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

#include "db_ido/hostgroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_DBTYPE(HostGroup, "hostgroup", DbObjectTypeHostGroup, "hostgroup_object_id", HostGroupDbObject);

HostGroupDbObject::HostGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr HostGroupDbObject::GetConfigFields() const
{
	Dictionary::Ptr fields = new Dictionary();
	HostGroup::Ptr group = static_pointer_cast<HostGroup>(GetObject());

	fields->Set("alias", group->GetDisplayName());
	fields->Set("notes", group->GetNotes());
	fields->Set("notes_url", group->GetNotesUrl());
	fields->Set("action_url", group->GetActionUrl());

	return fields;
}

Dictionary::Ptr HostGroupDbObject::GetStatusFields() const
{
	return nullptr;
}
