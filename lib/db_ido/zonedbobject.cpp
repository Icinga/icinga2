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
	Dictionary::Ptr fields = new Dictionary();
	Zone::Ptr zone = static_pointer_cast<Zone>(GetObject());

	fields->Set("is_global", zone->IsGlobal() ? 1 : 0);
	fields->Set("parent_zone_object_id", zone->GetParent());

	return fields;
}

Dictionary::Ptr ZoneDbObject::GetStatusFields() const
{
	Zone::Ptr zone = static_pointer_cast<Zone>(GetObject());

	Log(LogDebug, "ZoneDbObject")
		<< "update status for zone '" << zone->GetName() << "'";

	Dictionary::Ptr fields = new Dictionary();
	fields->Set("parent_zone_object_id", zone->GetParent());

	return fields;
}
