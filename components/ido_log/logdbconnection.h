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

#ifndef LOGDBCONNECTION_H
#define LOGDBCONNECTION_H

#include "base/dynamictype.h"
#include "ido/dbconnection.h"
#include <vector>

namespace icinga
{

/**
 * A MySQL database connection.
 *
 * @ingroup ido
 */
class LogDbConnection : public DbConnection
{
public:
	typedef shared_ptr<LogDbConnection> Ptr;
	typedef weak_ptr<LogDbConnection> WeakPtr;

	LogDbConnection(const Dictionary::Ptr& serializedUpdate);

	virtual void UpdateObject(const DbObject::Ptr& dbobj, DbUpdateType kind);
};

}

#endif /* LOGDBCONNECTION_H */
