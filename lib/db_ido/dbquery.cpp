// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "db_ido/dbquery.hpp"
#include "base/initialize.hpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

INITIALIZE_ONCE(&DbQuery::StaticInitialize);

std::map<String, int> DbQuery::m_CategoryFilterMap;

void DbQuery::StaticInitialize()
{
	ScriptGlobal::Set("Icinga.DbCatConfig", DbCatConfig);
	ScriptGlobal::Set("Icinga.DbCatState", DbCatState);
	ScriptGlobal::Set("Icinga.DbCatAcknowledgement", DbCatAcknowledgement);
	ScriptGlobal::Set("Icinga.DbCatComment", DbCatComment);
	ScriptGlobal::Set("Icinga.DbCatDowntime", DbCatDowntime);
	ScriptGlobal::Set("Icinga.DbCatEventHandler", DbCatEventHandler);
	ScriptGlobal::Set("Icinga.DbCatExternalCommand", DbCatExternalCommand);
	ScriptGlobal::Set("Icinga.DbCatFlapping", DbCatFlapping);
	ScriptGlobal::Set("Icinga.DbCatCheck", DbCatCheck);
	ScriptGlobal::Set("Icinga.DbCatLog", DbCatLog);
	ScriptGlobal::Set("Icinga.DbCatNotification", DbCatNotification);
	ScriptGlobal::Set("Icinga.DbCatProgramStatus", DbCatProgramStatus);
	ScriptGlobal::Set("Icinga.DbCatRetention", DbCatRetention);
	ScriptGlobal::Set("Icinga.DbCatStateHistory", DbCatStateHistory);

	ScriptGlobal::Set("Icinga.DbCatEverything", DbCatEverything);

	m_CategoryFilterMap["DbCatConfig"] = DbCatConfig;
	m_CategoryFilterMap["DbCatState"] = DbCatState;
	m_CategoryFilterMap["DbCatAcknowledgement"] = DbCatAcknowledgement;
	m_CategoryFilterMap["DbCatComment"] = DbCatComment;
	m_CategoryFilterMap["DbCatDowntime"] = DbCatDowntime;
	m_CategoryFilterMap["DbCatEventHandler"] = DbCatEventHandler;
	m_CategoryFilterMap["DbCatExternalCommand"] = DbCatExternalCommand;
	m_CategoryFilterMap["DbCatFlapping"] = DbCatFlapping;
	m_CategoryFilterMap["DbCatCheck"] = DbCatCheck;
	m_CategoryFilterMap["DbCatLog"] = DbCatLog;
	m_CategoryFilterMap["DbCatNotification"] = DbCatNotification;
	m_CategoryFilterMap["DbCatProgramStatus"] = DbCatProgramStatus;
	m_CategoryFilterMap["DbCatRetention"] = DbCatRetention;
	m_CategoryFilterMap["DbCatStateHistory"] = DbCatStateHistory;
	m_CategoryFilterMap["DbCatEverything"] = DbCatEverything;
}

const std::map<String, int>& DbQuery::GetCategoryFilterMap()
{
	return m_CategoryFilterMap;
}
