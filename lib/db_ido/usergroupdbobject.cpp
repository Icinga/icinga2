// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "db_ido/usergroupdbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/objectlock.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_DBTYPE(UserGroup, "contactgroup", DbObjectTypeContactGroup, "contactgroup_object_id", UserGroupDbObject);

UserGroupDbObject::UserGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr UserGroupDbObject::GetConfigFields() const
{
	UserGroup::Ptr group = static_pointer_cast<UserGroup>(GetObject());

	return new Dictionary({
		{ "alias", group->GetDisplayName() }
	});
}

Dictionary::Ptr UserGroupDbObject::GetStatusFields() const
{
	return nullptr;
}
