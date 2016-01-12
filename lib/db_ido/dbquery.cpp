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

#include "db_ido/dbquery.hpp"
#include "base/initialize.hpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

INITIALIZE_ONCE(&DbQuery::StaticInitialize);

void DbQuery::StaticInitialize(void)
{
	ScriptGlobal::Set("DbCatConfig", DbCatConfig);
	ScriptGlobal::Set("DbCatState", DbCatState);
	ScriptGlobal::Set("DbCatAcknowledgement", DbCatAcknowledgement);
	ScriptGlobal::Set("DbCatComment", DbCatComment);
	ScriptGlobal::Set("DbCatDowntime", DbCatDowntime);
	ScriptGlobal::Set("DbCatEventHandler", DbCatEventHandler);
	ScriptGlobal::Set("DbCatExternalCommand", DbCatExternalCommand);
	ScriptGlobal::Set("DbCatFlapping", DbCatFlapping);
	ScriptGlobal::Set("DbCatCheck", DbCatCheck);
	ScriptGlobal::Set("DbCatLog", DbCatLog);
	ScriptGlobal::Set("DbCatNotification", DbCatNotification);
	ScriptGlobal::Set("DbCatProgramStatus", DbCatProgramStatus);
	ScriptGlobal::Set("DbCatRetention", DbCatRetention);
	ScriptGlobal::Set("DbCatStateHistory", DbCatStateHistory);

	ScriptGlobal::Set("DbCatEverything", ~(unsigned int)0);
}
