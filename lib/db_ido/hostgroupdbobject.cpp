/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	HostGroup::Ptr group = static_pointer_cast<HostGroup>(GetObject());

	return new Dictionary({
		{ "alias", group->GetDisplayName() },
		{ "notes", group->GetNotes() },
		{ "notes_url", group->GetNotesUrl() },
		{ "action_url", group->GetActionUrl() }
	});
}

Dictionary::Ptr HostGroupDbObject::GetStatusFields() const
{
	return nullptr;
}
