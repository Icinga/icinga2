// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
