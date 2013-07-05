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

#include "ido_log/logdbconnection.h"
#include "base/logger_fwd.h"

using namespace icinga;

REGISTER_TYPE(LogDbConnection);

LogDbConnection::LogDbConnection(const Dictionary::Ptr& serializedUpdate)
	: DbConnection(serializedUpdate)
{
	/* We're already "connected" to our database... Let's do this. */
	UpdateAllObjects();
}

void LogDbConnection::UpdateObject(const DbObject::Ptr& dbobj, DbUpdateType kind)
{
	Log(LogInformation, "ido_log", "Object '" + dbobj->GetObject()->GetName() + "' was updated.");
}
