/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef DBQUERY_H
#define DBQUERY_H

#include "db_ido/i2-db_ido.hpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/customvarobject.hpp"
#include "base/dictionary.hpp"
#include "base/configobject.hpp"

namespace icinga
{

enum DbQueryType
{
	DbQueryInsert = 1,
	DbQueryUpdate = 2,
	DbQueryDelete = 4,
	DbQueryNewTransaction = 8
};

enum DbQueryCategory
{
	DbCatInvalid = -1,

	DbCatConfig = (1 << 0),
	DbCatState = (1 << 1),

	DbCatAcknowledgement = (1 << 2),
	DbCatComment = (1 << 3),
	DbCatDowntime = (1 << 4),
	DbCatEventHandler = (1 << 5),
	DbCatExternalCommand = (1 << 6),
	DbCatFlapping = (1 << 7),
	DbCatCheck = (1 << 8),
	DbCatLog = (1 << 9),
	DbCatNotification = (1 << 10),
	DbCatProgramStatus = (1 << 11),
	DbCatRetention = (1 << 12),
	DbCatStateHistory = (1 << 13)
};

class DbObject;

struct I2_DB_IDO_API DbQuery
{
	int Type;
	DbQueryCategory Category;
	String Table;
	String IdColumn;
	Dictionary::Ptr Fields;
	Dictionary::Ptr WhereCriteria;
	intrusive_ptr<DbObject> Object;
	DbValue::Ptr NotificationInsertID;
	bool ConfigUpdate;
	bool StatusUpdate;
	WorkQueuePriority Priority;

	static void StaticInitialize(void);

	DbQuery(void)
		: Type(0), Category(DbCatInvalid), ConfigUpdate(false), StatusUpdate(false), Priority(PriorityLow)
	{ }
};

}

#endif /* DBQUERY_H */

#include "db_ido/dbobject.hpp"
