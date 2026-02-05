// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "db_ido/zonedbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "base/logger.hpp"

using namespace icinga;


REGISTER_DBTYPE(Zone, "zone", DbObjectTypeZone, "zone_object_id", ZoneDbObject);

ZoneDbObject::ZoneDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

Dictionary::Ptr ZoneDbObject::GetConfigFields() const
{
	Zone::Ptr zone = static_pointer_cast<Zone>(GetObject());

	return new Dictionary({
		{ "is_global", zone->IsGlobal() ? 1 : 0 },
		{ "parent_zone_object_id", zone->GetParent() }

	});
}

Dictionary::Ptr ZoneDbObject::GetStatusFields() const
{
	Zone::Ptr zone = static_pointer_cast<Zone>(GetObject());

	Log(LogDebug, "ZoneDbObject")
		<< "update status for zone '" << zone->GetName() << "'";

	return new Dictionary({
		{ "parent_zone_object_id", zone->GetParent() }
	});
}
