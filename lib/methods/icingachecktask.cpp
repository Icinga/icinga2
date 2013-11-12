/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "methods/icingachecktask.h"
#include "icinga/cib.h"
#include "base/application.h"
#include "base/utility.h"
#include "base/scriptfunction.h"

using namespace icinga;

REGISTER_SCRIPTFUNCTION(IcingaCheck, &IcingaCheckTask::ScriptFunc);

CheckResult::Ptr IcingaCheckTask::ScriptFunc(const Service::Ptr&)
{
	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	Dictionary::Ptr perfdata = make_shared<Dictionary>();
	perfdata->Set("active_checks", CIB::GetActiveChecksStatistics(interval) / interval);
	perfdata->Set("passive_checks", CIB::GetPassiveChecksStatistics(interval) / interval);

	CheckResult::Ptr cr = make_shared<CheckResult>();
	cr->SetOutput("Icinga 2 is running.");
	cr->SetPerformanceData(perfdata);
	cr->SetState(StateOK);

	return cr;
}

