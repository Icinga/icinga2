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

#include "ido/dbobject.h"
#include "ido/dbtype.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>

using namespace icinga;

boost::signals2::signal<void (const DbObject::Ptr&, DbUpdateType)> DbObject::OnObjectUpdated;

DbObject::DbObject(const shared_ptr<DbType>& type, const String& name1, const String& name2)
	: m_Name1(name1), m_Name2(name2), m_Type(type)
{ }

void DbObject::StaticInitialize(void)
{
	DynamicObject::OnRegistered.connect(boost::bind(&DbObject::ObjectRegisteredHandler, _1));
	DynamicObject::OnUnregistered.connect(boost::bind(&DbObject::ObjectUnregisteredHandler, _1));
	DynamicObject::OnTransactionClosing.connect(boost::bind(&DbObject::TransactionClosingHandler, _1, _2));
	DynamicObject::OnFlushObject.connect(boost::bind(&DbObject::FlushObjectHandler, _1, _2));
}

void DbObject::SetObject(const DynamicObject::Ptr& object)
{
	m_Object = object;
}

DynamicObject::Ptr DbObject::GetObject(void) const
{
	return m_Object;
}

String DbObject::GetName1(void) const
{
	return m_Name1;
}

String DbObject::GetName2(void) const
{
	return m_Name2;
}

DbType::Ptr DbObject::GetType(void) const
{
	return m_Type;
}

void DbObject::SendUpdate(DbUpdateType kind)
{
	OnObjectUpdated(GetSelf(), kind);
}

DbObject::Ptr DbObject::GetOrCreateByObject(const DynamicObject::Ptr& object)
{
	DbObject::Ptr dbobj = static_pointer_cast<DbObject>(object->GetExtension("DbObject"));

	if (dbobj)
		return dbobj;

	DbType::Ptr dbtype = DbType::GetByName(object->GetType()->GetName());

	if (!dbtype)
		return DbObject::Ptr();

	Service::Ptr service;
	String name1, name2;

	service = dynamic_pointer_cast<Service>(object);

	if (service) {
		Host::Ptr host = service->GetHost();

		if (!host)
			return DbObject::Ptr();

		name1 = service->GetHost()->GetName();
		name2 = service->GetShortName();
	} else {
		name1 = object->GetName();
	}

	dbobj = dbtype->GetOrCreateObjectByName(object->GetName(), String());

	{
		ObjectLock olock(object);
		dbobj->SetObject(object);
		object->SetExtension("DbObject", dbobj);
	}

	return dbobj;
}

void DbObject::ObjectRegisteredHandler(const DynamicObject::Ptr& object)
{
	DbObject::Ptr dbobj = GetOrCreateByObject(object);

	if (!dbobj)
		return;

	dbobj->SendUpdate(DbObjectCreated);
}

void DbObject::ObjectUnregisteredHandler(const DynamicObject::Ptr& object)
{
	DbObject::Ptr dbobj = GetOrCreateByObject(object);

	if (!dbobj)
		return;

	dbobj->SendUpdate(DbObjectRemoved);

	{
		ObjectLock olock(object);
		object->ClearExtension("DbObject");
	}
}

void DbObject::TransactionClosingHandler(double tx, const std::set<DynamicObject::WeakPtr>& modifiedObjects)
{
        BOOST_FOREACH(const DynamicObject::WeakPtr& wobject, modifiedObjects) {
                DynamicObject::Ptr object = wobject.lock();

                if (!object)
                        continue;

                FlushObjectHandler(tx, object);
        }
}

void DbObject::FlushObjectHandler(double tx, const DynamicObject::Ptr& object)
{
	DbObject::Ptr dbobj = GetOrCreateByObject(object);

	if (!dbobj)
		return;

	dbobj->SendUpdate();
}
