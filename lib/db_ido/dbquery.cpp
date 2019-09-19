/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/dbquery.hpp"
#include "base/initialize.hpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

INITIALIZE_ONCE(&DbQuery::StaticInitialize);

std::map<String, int> DbQuery::m_CategoryFilterMap;

void DbQuery::StaticInitialize()
{
	ScriptGlobal::Set("Icinga.DbCatConfig", DbCatConfig, true);
	ScriptGlobal::Set("Icinga.DbCatState", DbCatState, true);
	ScriptGlobal::Set("Icinga.DbCatAcknowledgement", DbCatAcknowledgement, true);
	ScriptGlobal::Set("Icinga.DbCatComment", DbCatComment, true);
	ScriptGlobal::Set("Icinga.DbCatDowntime", DbCatDowntime, true);
	ScriptGlobal::Set("Icinga.DbCatEventHandler", DbCatEventHandler, true);
	ScriptGlobal::Set("Icinga.DbCatExternalCommand", DbCatExternalCommand, true);
	ScriptGlobal::Set("Icinga.DbCatFlapping", DbCatFlapping, true);
	ScriptGlobal::Set("Icinga.DbCatCheck", DbCatCheck, true);
	ScriptGlobal::Set("Icinga.DbCatLog", DbCatLog, true);
	ScriptGlobal::Set("Icinga.DbCatNotification", DbCatNotification, true);
	ScriptGlobal::Set("Icinga.DbCatProgramStatus", DbCatProgramStatus, true);
	ScriptGlobal::Set("Icinga.DbCatRetention", DbCatRetention, true);
	ScriptGlobal::Set("Icinga.DbCatStateHistory", DbCatStateHistory, true);

	ScriptGlobal::Set("Icinga.DbCatEverything", DbCatEverything, true);

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
