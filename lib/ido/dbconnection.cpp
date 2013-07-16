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

#include "ido/dbconnection.h"
#include "base/dynamictype.h"
#include <boost/foreach.hpp>

using namespace icinga;

DbConnection::DbConnection(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{ }

void DbConnection::Initialize(void)
{
	DbObject::OnObjectUpdated.connect(boost::bind(&DbConnection::UpdateObject, this, _1, _2));
}

void DbConnection::SetReference(const DbObject::Ptr& dbobj, const DbReference& dbref)
{
	if (dbref.IsValid())
		m_References[dbobj] = dbref;
	else
		m_References.erase(dbobj);
}

DbReference DbConnection::GetReference(const DbObject::Ptr& dbobj) const
{
	std::map<DbObject::Ptr, DbReference>::const_iterator it;

	it = m_References.find(dbobj);

	if (it == m_References.end())
		return DbReference();

	return it->second;
}

void DbConnection::UpdateObject(const DbObject::Ptr&, DbUpdateType)
{
	/* Default handler does nothing. */
}

void DbConnection::UpdateAllObjects(void)
{
	DynamicType::Ptr type;
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			DbObject::Ptr dbobj = DbObject::GetOrCreateByObject(object);

			if (dbobj)
				UpdateObject(dbobj, DbObjectCreated);
		}
	}
}
