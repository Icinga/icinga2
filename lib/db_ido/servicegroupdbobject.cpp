// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "db_ido/servicegroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"

using namespace icinga;

REGISTER_DBTYPE(ServiceGroup, "servicegroup", DbObjectTypeServiceGroup, "servicegroup_object_id", ServiceGroupDbObject);

ServiceGroupDbObject::ServiceGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr ServiceGroupDbObject::GetConfigFields() const
{
	ServiceGroup::Ptr group = static_pointer_cast<ServiceGroup>(GetObject());

	return new Dictionary({
		{ "alias", group->GetDisplayName() },
		{ "notes", group->GetNotes() },
		{ "notes_url", group->GetNotesUrl() },
		{ "action_url", group->GetActionUrl() }
	});
}

Dictionary::Ptr ServiceGroupDbObject::GetStatusFields() const
{
	return nullptr;
}
