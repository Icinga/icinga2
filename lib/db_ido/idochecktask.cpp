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

#include "db_ido/idochecktask.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/perfdatavalue.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/configtype.hpp"
#include "base/convert.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(IdoCheck, &IdoCheckTask::ScriptFunc);

void IdoCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();
	Value raw_command = commandObj->GetCommandLine();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("command", commandObj));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	String idoType = MacroProcessor::ResolveMacros("$ido_type$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (idoType.IsEmpty()) {
		cr->SetOutput("Macro 'ido_type' must be set.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	String idoName = MacroProcessor::ResolveMacros("$ido_name$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (idoName.IsEmpty()) {
		cr->SetOutput("Macro 'ido_name' must be set.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	Type::Ptr type = Type::GetByName(idoType);

	if (!type || !DbConnection::TypeInstance->IsAssignableFrom(type)) {
		cr->SetOutput("IDO type '" + idoType + "' is invalid.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	ConfigType::Ptr dtype = ConfigType::GetByName(idoType);
	VERIFY(dtype);

	DbConnection::Ptr conn = static_pointer_cast<DbConnection>(dtype->GetObject(idoName));

	if (!conn) {
		cr->SetOutput("IDO connection '" + idoName + "' does not exist.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	double qps = conn->GetQueryCount(60) / 60.0;

	if (!conn->GetConnected()) {
		if (conn->GetShouldConnect()) {
			cr->SetOutput("Could not connect to the database server.");
			cr->SetState(ServiceCritical);
		} else {
			cr->SetOutput("Not currently enabled: Another cluster instance is responsible for the IDO database.");
			cr->SetState(ServiceOK);
		}
	} else {
		String schema_version = conn->GetSchemaVersion();

		if (Utility::CompareVersion(IDO_CURRENT_SCHEMA_VERSION, schema_version) < 0) {
			cr->SetOutput("Outdated schema version: " + schema_version + "; Latest version: "  IDO_CURRENT_SCHEMA_VERSION);
			cr->SetState(ServiceWarning);
		} else {
			std::ostringstream msgbuf;
			msgbuf << "Connected to the database server; queries per second: "  << std::fixed << std::setprecision(3) << qps;
			cr->SetOutput(msgbuf.str());
			cr->SetState(ServiceOK);
		}
	}

	Array::Ptr perfdata = new Array();
	perfdata->Add(new PerfdataValue("queries", qps));
	perfdata->Add(new PerfdataValue("queries_1min", conn->GetQueryCount(60)));
	perfdata->Add(new PerfdataValue("queries_5mins", conn->GetQueryCount(5 * 60)));
	perfdata->Add(new PerfdataValue("queries_15mins", conn->GetQueryCount(15 * 60)));
	perfdata->Add(new PerfdataValue("pending_queries", conn->GetPendingQueryCount()));
	cr->SetPerformanceData(perfdata);

	checkable->ProcessCheckResult(cr);
}
