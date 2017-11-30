/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/configtype.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, IdoCheck, &IdoCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

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
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String idoType = MacroProcessor::ResolveMacros("$ido_type$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String idoName = MacroProcessor::ResolveMacros("$ido_name$", resolvers, checkable->GetLastCheckResult(),
	    NULL, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (idoType.IsEmpty()) {
		cr->SetOutput("Macro 'ido_type' must be set.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

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

	ConfigType *dtype = dynamic_cast<ConfigType *>(type.get());
	VERIFY(dtype);

	DbConnection::Ptr conn = static_pointer_cast<DbConnection>(dtype->GetObject(idoName));

	if (!conn) {
		cr->SetOutput("IDO connection '" + idoName + "' does not exist.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	double qps = conn->GetQueryCount(60) / 60.0;

	if (conn->IsPaused()) {
		cr->SetOutput("IDO connection is temporarily disabled on this cluster instance.");
		cr->SetState(ServiceOK);
		checkable->ProcessCheckResult(cr);
		return;
	}

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
		std::ostringstream msgbuf;

		if (Utility::CompareVersion(IDO_CURRENT_SCHEMA_VERSION, schema_version) < 0) {
			msgbuf << "Outdated schema version: '" << schema_version << "'. Latest version: '" << IDO_CURRENT_SCHEMA_VERSION << "'.";
			cr->SetState(ServiceWarning);
		} else {
			msgbuf << "Connected to the database server (Schema version: '" << schema_version << "').";
			cr->SetState(ServiceOK);
		}

		msgbuf << " Queries per second: " << std::fixed << std::setprecision(3) << qps;

		cr->SetOutput(msgbuf.str());
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
