/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/agentupdateconfigcommand.hpp"
#include "cli/agentutility.hpp"
#include "cli/repositoryutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("agent/update-config", AgentUpdateConfigCommand);

String AgentUpdateConfigCommand::GetDescription(void) const
{
	return "Update Icinga 2 agent config.";
}

String AgentUpdateConfigCommand::GetShortDescription(void) const
{
	return "update agent config";
}

ImpersonationLevel AgentUpdateConfigCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

/**
 * The entry point for the "agent update-config" CLI command.
 *
 * @returns An exit status.
 */
int AgentUpdateConfigCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	//If there are changes pending, abort the current operation
	if (RepositoryUtility::ChangeLogHasPendingChanges()) {
		Log(LogWarning, "cli")
		    << "There are pending changes for commit.\n"
		    << "Please review and commit them using 'icinga2 repository commit [--simulate]\n"
		    << "or drop them using 'icinga2 repository commit --clear' before proceeding.";
		return 1;
	}

	Log(LogInformation, "cli")
	    << "Updating agent configuration for ";

	AgentUtility::PrintAgents(std::cout);

	BOOST_FOREACH(const Dictionary::Ptr& agent, AgentUtility::GetAgents()) {
		Dictionary::Ptr repository = agent->Get("repository");
		String zone = agent->Get("zone");
		String endpoint = agent->Get("endpoint");

		ObjectLock olock(repository);
		BOOST_FOREACH(const Dictionary::Pair& kv, repository) {
			String host = kv.first;

			/* add a new host to the config repository */
			Dictionary::Ptr host_attrs = make_shared<Dictionary>();
			host_attrs->Set("check_command", "dummy"); //TODO: add a repository-host template
			host_attrs->Set("zone", zone);

			if (!RepositoryUtility::AddObject(host, "Host", host_attrs)) {
				Log(LogCritical, "cli")
				    << "Cannot add agent host '" << host << "' to the config repository!\n";
				continue;
			}

			Array::Ptr services = kv.second;
			ObjectLock xlock(services);
			BOOST_FOREACH(const String& service, services) {

				/* add a new service for this host to the config repository */
				Dictionary::Ptr service_attrs = make_shared<Dictionary>();
				service_attrs->Set("host_name", host); //Required for host-service relation
				service_attrs->Set("check_command", "dummy"); //TODO: add a repository-service template
				service_attrs->Set("zone", zone);

				if (!RepositoryUtility::AddObject(service, "Service", service_attrs)) {
					Log(LogCritical, "cli")
					    << "Cannot add agent host '" << host << "' to the config repository!\n";
					continue;
				}
			}
		}

		/* write a new zone and endpoint for the agent */
		Dictionary::Ptr endpoint_attrs = make_shared<Dictionary>();

		Dictionary::Ptr settings = agent->Get("settings");

		if (settings) {
			if (settings->Contains("host"))
				endpoint_attrs->Set("host", settings->Get("host"));
			if (settings->Contains("port"))
				endpoint_attrs->Set("port", settings->Get("port"));
		}

		if (!RepositoryUtility::AddObject(endpoint, "Endpoint", endpoint_attrs)) {
			Log(LogCritical, "cli")
			    << "Cannot add agent endpoint '" << endpoint << "' to the config repository!\n";
		}

		Dictionary::Ptr zone_attrs = make_shared<Dictionary>();
		Array::Ptr zone_members = make_shared<Array>();

		zone_members->Add(endpoint);
		zone_attrs->Set("endpoints", zone_members);
		zone_attrs->Set("parent", agent->Get("parent_zone"));

		if (!RepositoryUtility::AddObject(zone, "Zone", zone_attrs)) {
			Log(LogCritical, "cli")
			    << "Cannot add agent zone '" << zone << "' to the config repository!\n";
		}
	}

	Log(LogInformation, "cli", "Committing agent configuration.");

	RepositoryUtility::PrintChangeLog(std::cout);
	std::cout << "\n";
	RepositoryUtility::CommitChangeLog();

	std::cout << "Make sure to reload Icinga 2 for these changes to take effect." << std::endl;

	return 0;
}
