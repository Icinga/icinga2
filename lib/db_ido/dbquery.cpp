/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/scriptvariable.hpp"

using namespace icinga;

INITIALIZE_ONCE(&DbQuery::StaticInitialize);

void DbQuery::StaticInitialize(void)
{
	ScriptVariable::Set("DbCatConfig", DbCatConfig, true, true);
	ScriptVariable::Set("DbCatState", DbCatState, true, true);
	ScriptVariable::Set("DbCatAcknowledgement", DbCatAcknowledgement, true, true);
	ScriptVariable::Set("DbCatComment", DbCatComment, true, true);
	ScriptVariable::Set("DbCatDowntime", DbCatDowntime, true, true);
	ScriptVariable::Set("DbCatEventHandler", DbCatEventHandler, true, true);
	ScriptVariable::Set("DbCatExternalCommand", DbCatExternalCommand, true, true);
	ScriptVariable::Set("DbCatFlapping", DbCatFlapping, true, true);
	ScriptVariable::Set("DbCatCheck", DbCatCheck, true, true);
	ScriptVariable::Set("DbCatLog", DbCatLog, true, true);
	ScriptVariable::Set("DbCatNotification", DbCatNotification, true, true);
	ScriptVariable::Set("DbCatProgramStatus", DbCatProgramStatus, true, true);
	ScriptVariable::Set("DbCatRetention", DbCatRetention, true, true);
	ScriptVariable::Set("DbCatStateHistory", DbCatStateHistory, true, true);

	ScriptVariable::Set("DbCatEverything", ~(unsigned int)0, true, true);
}
