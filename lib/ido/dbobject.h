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

#ifndef DBOBJECT_H
#define DBOBJECT_H

#include "ido/dbreference.h"
#include "ido/dbquery.h"
#include "base/dynamicobject.h"
#include <boost/smart_ptr/make_shared.hpp>

namespace icinga
{

enum DbObjectUpdateType
{
	DbObjectCreated,
	DbObjectRemoved
};

class DbType;

/**
 * A database object.
 *
 * @ingroup ido
 */
class DbObject : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(DbObject);

	void SetObject(const DynamicObject::Ptr& object);
	DynamicObject::Ptr GetObject(void) const;

	String GetName1(void) const;
	String GetName2(void) const;
	boost::shared_ptr<DbType> GetType(void) const;

	virtual Dictionary::Ptr GetConfigFields(void) const = 0;
	virtual Dictionary::Ptr GetStatusFields(void) const = 0;

	static DbObject::Ptr GetOrCreateByObject(const DynamicObject::Ptr& object);

	static boost::signals2::signal<void (const DbObject::Ptr&)> OnRegistered;
	static boost::signals2::signal<void (const DbObject::Ptr&)> OnUnregistered;
	static boost::signals2::signal<void (const DbQuery&)> OnQuery;

	void SendConfigUpdate(void);
	void SendStatusUpdate(void);

protected:
	DbObject(const boost::shared_ptr<DbType>& type, const String& name1, const String& name2);

private:
	String m_Name1;
	String m_Name2;
	boost::shared_ptr<DbType> m_Type;
	DynamicObject::Ptr m_Object;

	friend boost::shared_ptr<DbObject> boost::make_shared<>(const icinga::String&, const icinga::String&);

	static void StaticInitialize(void);

	static void ObjectRegisteredHandler(const DynamicObject::Ptr& object);
	static void ObjectUnregisteredHandler(const DynamicObject::Ptr& object);
	//static void TransactionClosingHandler(double tx, const std::set<DynamicObject::WeakPtr>& modifiedObjects);
	//static void FlushObjectHandler(double tx, const DynamicObject::Ptr& object);

	friend class DbType;
};

}

#endif /* DBOBJECT_H */
