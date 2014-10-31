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
#include "base/json.hpp"
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
		    << "Please review and commit them using 'icinga2 repository commit [--simulate]'\n"
		    << "or drop them using 'icinga2 repository commit --clear' before proceeding.";
		return 1;
	}

	String inventory_path = AgentUtility::GetRepositoryPath() + "/inventory.index";

	Dictionary::Ptr old_inventory = make_shared<Dictionary>();
	if (Utility::PathExists(inventory_path)) {
		old_inventory = Utility::LoadJsonFile(inventory_path);
	}

	Dictionary::Ptr inventory = make_shared<Dictionary>();

	Log(LogInformation, "cli")
	    << "Updating agent configuration for ";

	AgentUtility::PrintAgents(std::cout);

	Utility::LoadExtensionLibrary("icinga");

	std::vector<String> object_paths = RepositoryUtility::GetObjects();

	BOOST_FOREACH(const Dictionary::Ptr& agent, AgentUtility::GetAgents()) {
		Dictionary::Ptr repository = agent->Get("repository");
		String zone = agent->Get("zone");
		String endpoint = agent->Get("endpoint");
		String agent_name = endpoint;

		/* store existing structure in index */
		inventory->Set(endpoint, agent);

		Dictionary::Ptr host_services = make_shared<Dictionary>();

		Log(LogInformation, "cli")
		    << "Repository for agent '" << endpoint << "' does not contain a health check host. Adding host '" << zone << "'.";

		Dictionary::Ptr host_attrs = make_shared<Dictionary>();
		host_attrs->Set("__name", zone);
		host_attrs->Set("name", zone);
		host_attrs->Set("check_command", "cluster-zone");
		Array::Ptr host_imports = make_shared<Array>();
		host_imports->Add("agent-host"); //default host agent template
		host_attrs->Set("import", host_imports);

		if (!RepositoryUtility::AddObject(zone, "Host", host_attrs)) {
			Log(LogCritical, "cli")
			    << "Cannot add agent host '" << zone << "' to the config repository!\n";
		}

		ObjectLock olock(repository);
		BOOST_FOREACH(const Dictionary::Pair& kv, repository) {
			String host = kv.first;
			String host_pattern = host + ".conf";
			bool skip_host = false;

			if (host == "localhost") {
				Log(LogWarning, "cli")
				    << "Ignoring host '" << host << "'. Please make sure to configure a unique name on your agent '" << agent_name << "'.";
				continue;
			}

			BOOST_FOREACH(const String& object_path, object_paths) {
				if (object_path.Contains(host_pattern)) {
					Log(LogWarning, "cli")
					    << "Host '" << host << "' already existing. Skipping its creation.";
					skip_host = true;
					break;
				}
			}

			/* host has already been created above */
			if (host == zone)
				skip_host = true;

			/* check against black/whitelist before trying to add host */
			if (AgentUtility::CheckAgainstBlackAndWhiteList("blacklist", agent_name, host, Empty) &&
			    !AgentUtility::CheckAgainstBlackAndWhiteList("whitelist", agent_name, host, Empty)) {
				Log(LogWarning, "cli")
				    << "Host '" << host << "' on agent '" << agent_name << "' is blacklisted, but not whitelisted. Skipping.";
				skip_host = true;
			}

			if (!skip_host) {
				/* add a new host to the config repository */
				Dictionary::Ptr host_attrs = make_shared<Dictionary>();
				host_attrs->Set("__name", host);
				host_attrs->Set("name", host);

				if (host == zone)
					host_attrs->Set("check_command", "cluster-zone");
				else {
					host_attrs->Set("check_command", "dummy");
					host_attrs->Set("zone", zone);
				}

				Array::Ptr host_imports = make_shared<Array>();
				host_imports->Add("agent-host"); //default host agent template
				host_attrs->Set("import", host_imports);

				if (!RepositoryUtility::AddObject(host, "Host", host_attrs)) {
					Log(LogCritical, "cli")
					    << "Cannot add agent host '" << host << "' to the config repository!\n";
				}
			}

			Array::Ptr services = kv.second;

			if (services->GetLength() == 0) {
				Log(LogNotice, "cli")
				    << "Host '" << host << "' without services.";
				continue;
			}

			ObjectLock xlock(services);
			BOOST_FOREACH(const String& service, services) {
				bool skip_service = false;

				String service_pattern = host + "/" + service + ".conf";

				BOOST_FOREACH(const String& object_path, object_paths) {
					if (object_path.Contains(service_pattern)) {
						Log(LogWarning, "cli")
						    << "Service '" << service << "' on Host '" << host << "' already existing. Skipping its creation.";
						skip_service = true;
						break;
					}
				}

				/* check against black/whitelist before trying to add service */
				if (AgentUtility::CheckAgainstBlackAndWhiteList("blacklist", endpoint, host, service) &&
				    !AgentUtility::CheckAgainstBlackAndWhiteList("whitelist", endpoint, host, service)) {
					Log(LogWarning, "cli")
					    << "Service '" << service << "' on host '" << host << "' on agent '"
					    << agent_name << "' is blacklisted, but not whitelisted. Skipping.";
					skip_service = true;
				}

				if (skip_service)
					continue;

				/* add a new service for this host to the config repository */
				Dictionary::Ptr service_attrs = make_shared<Dictionary>();
				String long_name = host + "!" + service; //use NameComposer?
				service_attrs->Set("__name", long_name);
				service_attrs->Set("name", service);
				service_attrs->Set("host_name", host); //Required for host-service relation
				service_attrs->Set("check_command", "dummy");
				service_attrs->Set("zone", zone);

				Array::Ptr service_imports = make_shared<Array>();
				service_imports->Add("agent-service"); //default service agent template
				service_attrs->Set("import", service_imports);

				if (!RepositoryUtility::AddObject(service, "Service", service_attrs)) {
					Log(LogCritical, "cli")
					    << "Cannot add agent host '" << host << "' to the config repository!\n";
					continue;
				}
			}
		}

		/* write a new zone and endpoint for the agent */
		Dictionary::Ptr endpoint_attrs = make_shared<Dictionary>();
		endpoint_attrs->Set("__name", endpoint);
		endpoint_attrs->Set("name", endpoint);

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
		zone_attrs->Set("__name", zone);
		zone_attrs->Set("name", zone);
		zone_attrs->Set("endpoints", zone_members);

		String agent_parent_zone = "master"; //hardcode the name
		String parent_zone;

		if (!agent->Contains("parent_zone")) {
			Log(LogWarning, "cli")
			    << "Agent '" << endpoint << "' does not have any parent zone defined. Using 'master' as default. Please verify the generated configuration.";
			parent_zone = agent_parent_zone;
		} else {
			parent_zone = agent->Get("parent_zone");

			if (parent_zone.IsEmpty()) {
				Log(LogWarning, "cli")
				    << "Agent '" << endpoint << "' does not have any parent zone defined. Using 'master' as default. Please verify the generated configuration.";
				parent_zone = agent_parent_zone;
			}
		}

		zone_attrs->Set("parent", parent_zone);

		if (!RepositoryUtility::AddObject(zone, "Zone", zone_attrs)) {
			Log(LogCritical, "cli")
			    << "Cannot add agent zone '" << zone << "' to the config repository!\n";
		}
	}

	/* check if there are objects inside the old_inventory which do not exist anymore */
	BOOST_FOREACH(const Dictionary::Pair& old_agent_objs, old_inventory) {

		String old_agent_name = old_agent_objs.first;

		/* check if the agent was dropped */
		if (!inventory->Contains(old_agent_name)) {
			Log(LogInformation, "cli")
			    << "Agent update found old agent '" << old_agent_name << "'. Removing it and all of its hosts/services.";

			//TODO Remove an agent and all of his hosts
			Dictionary::Ptr old_agent = old_inventory->Get(old_agent_name);
			Dictionary::Ptr old_agent_repository = old_agent->Get("repository");

			ObjectLock olock(old_agent_repository);
			BOOST_FOREACH(const Dictionary::Pair& kv, old_agent_repository) {
				String host = kv.first;

				Dictionary::Ptr host_attrs = make_shared<Dictionary>();
				host_attrs->Set("name", host);
				RepositoryUtility::RemoveObject(host, "Host", host_attrs); //this removes all services for this host as well
			}

			String zone = old_agent->Get("zone");
			String endpoint = old_agent->Get("endpoint");

			Dictionary::Ptr zone_attrs = make_shared<Dictionary>();
			zone_attrs->Set("name", zone);
			RepositoryUtility::RemoveObject(zone, "Zone", zone_attrs);

			Dictionary::Ptr endpoint_attrs = make_shared<Dictionary>();
			endpoint_attrs->Set("name", endpoint);
			RepositoryUtility::RemoveObject(endpoint, "Endpoint", endpoint_attrs);
		} else {
			/* get the current agent */
			Dictionary::Ptr new_agent = inventory->Get(old_agent_name);
			Dictionary::Ptr new_agent_repository = new_agent->Get("repository");

			Dictionary::Ptr old_agent = old_inventory->Get(old_agent_name);
			Dictionary::Ptr old_agent_repository = old_agent->Get("repository");

			ObjectLock xlock(old_agent_repository);
			BOOST_FOREACH(const Dictionary::Pair& kv, old_agent_repository) {
				String old_host = kv.first;

				if (old_host == "localhost") {
					Log(LogWarning, "cli")
					    << "Ignoring host '" << old_host << "'. Please make sure to configure a unique name on your agent '" << old_agent << "'.";
					continue;
				}

				/* check against black/whitelist before trying to remove host */
				if (AgentUtility::CheckAgainstBlackAndWhiteList("blacklist", old_agent_name, old_host, Empty) &&
				    !AgentUtility::CheckAgainstBlackAndWhiteList("whitelist", old_agent_name, old_host, Empty)) {
					Log(LogWarning, "cli")
					    << "Host '" << old_agent << "' on agent '" << old_agent << "' is blacklisted, but not whitelisted. Skipping.";
					continue;
				}

				if (!new_agent_repository->Contains(old_host)) {
					Log(LogInformation, "cli")
					    << "Agent update found old host '" << old_host << "' on agent '" << old_agent_name << "'. Removing it.";

					Dictionary::Ptr host_attrs = make_shared<Dictionary>();
					host_attrs->Set("name", old_host);
					RepositoryUtility::RemoveObject(old_host, "Host", host_attrs); //this will remove all services for this host too
				} else {
					/* host exists, now check all services for this host */
					Array::Ptr old_services = kv.second;
					Array::Ptr new_services = new_agent_repository->Get(old_host);

					ObjectLock ylock(old_services);
					BOOST_FOREACH(const String& old_service, old_services) {
						/* check against black/whitelist before trying to remove service */
						if (AgentUtility::CheckAgainstBlackAndWhiteList("blacklist", old_agent_name, old_host, old_service) &&
						    !AgentUtility::CheckAgainstBlackAndWhiteList("whitelist", old_agent_name, old_host, old_service)) {
							Log(LogWarning, "cli")
							    << "Service '" << old_service << "' on host '" << old_host << "' on agent '"
							    << old_agent_name << "' is blacklisted, but not whitelisted. Skipping.";
							continue;
						}

						if (!new_services->Contains(old_service)) {
							Log(LogInformation, "cli")
							    << "Agent update found old service '" << old_service << "' on host '" << old_host
							    << "' on agent '" << old_agent_name << "'. Removing it.";

							Dictionary::Ptr service_attrs = make_shared<Dictionary>();
							service_attrs->Set("name", old_service);
							service_attrs->Set("host_name", old_host);
							RepositoryUtility::RemoveObject(old_service, "Service", service_attrs);
						}
					}
				}
			}
		}
	}

	Log(LogInformation, "cli", "Committing agent configuration.");

	RepositoryUtility::PrintChangeLog(std::cout);
	std::cout << "\n";
	RepositoryUtility::CommitChangeLog();

	/* store the new inventory for next run */
	Utility::SaveJsonFile(inventory_path, inventory);

	std::cout << "Make sure to reload Icinga 2 for these changes to take effect." << std::endl;

	return 0;
}
