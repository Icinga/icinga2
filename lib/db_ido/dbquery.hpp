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
	DbCatInvalid = 0, //-1 is required for DbCatEverything
	DbCatEverything = ~0,

	DbCatConfig = 1,
	DbCatState = 2,
	DbCatAcknowledgement = 4,
	DbCatComment = 8,
	DbCatDowntime = 16,
	DbCatEventHandler = 32,
	DbCatExternalCommand = 64,
	DbCatFlapping = 128,
	DbCatCheck = 256,
	DbCatLog = 512,
	DbCatNotification = 1024,
	DbCatProgramStatus = 2048,
	DbCatRetention = 4096,
	DbCatStateHistory = 8192
};

class DbObject;

struct DbQuery
{
	int Type{0};
	DbQueryCategory Category{DbCatInvalid};
	String Table;
	String IdColumn;
	Dictionary::Ptr Fields;
	Dictionary::Ptr WhereCriteria;
	intrusive_ptr<DbObject> Object;
	DbValue::Ptr NotificationInsertID;
	bool ConfigUpdate{false};
	bool StatusUpdate{false};
	WorkQueuePriority Priority{PriorityNormal};

	static void StaticInitialize();

	static const std::map<String, int>& GetCategoryFilterMap();

private:
	static std::map<String, int> m_CategoryFilterMap;
};

}

#endif /* DBQUERY_H */

#include "db_ido/dbobject.hpp"
