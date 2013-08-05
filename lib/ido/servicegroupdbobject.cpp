/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "ido/servicegroupdbobject.h"
#include "ido/dbtype.h"
#include "ido/dbvalue.h"
#include "icinga/servicegroup.h"
#include "base/objectlock.h"
#include "base/initialize.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_DBTYPE(ServiceGroup, "servicegroup", DbObjectTypeServiceGroup, "servicegroup_object_id", ServiceGroupDbObject);
INITIALIZE_ONCE(ServiceGroupDbObject, &ServiceGroupDbObject::StaticInitialize);

ServiceGroupDbObject::ServiceGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2)
	: DbObject(type, name1, name2)
{ }

void ServiceGroupDbObject::StaticInitialize(void)
{
	ServiceGroup::OnMembersChanged.connect(&ServiceGroupDbObject::MembersChangedHandler);
}

Dictionary::Ptr ServiceGroupDbObject::GetConfigFields(void) const
{
	Dictionary::Ptr fields = boost::make_shared<Dictionary>();
	ServiceGroup::Ptr group = static_pointer_cast<ServiceGroup>(GetObject());

	fields->Set("alias", group->GetDisplayName());

	return fields;
}

Dictionary::Ptr ServiceGroupDbObject::GetStatusFields(void) const
{
	return Empty;
}


void ServiceGroupDbObject::OnConfigUpdate(void)
{
	MembersChangedHandler();
}

void ServiceGroupDbObject::MembersChangedHandler(void)
{
	DbQuery query1;
	query1.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
	query1.Type = DbQueryDelete;
	query1.WhereCriteria = boost::make_shared<Dictionary>();
	query1.WhereCriteria->Set("instance_id", 0);
	OnQuery(query1);

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("ServiceGroup")) {
		ServiceGroup::Ptr sg = static_pointer_cast<ServiceGroup>(object);

		BOOST_FOREACH(const Service::Ptr& service, sg->GetMembers()) {
			DbQuery query2;
			query2.Table = DbType::GetByName("ServiceGroup")->GetTable() + "_members";
			query2.Type = DbQueryInsert;
			query2.Fields = boost::make_shared<Dictionary>();
			query2.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
			query2.Fields->Set("servicegroup_id", DbValue::FromObjectInsertID(sg));
			query2.Fields->Set("service_object_id", service);
			OnQuery(query2);
		}
	}
}
