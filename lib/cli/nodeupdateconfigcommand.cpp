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

#include "cli/nodeupdateconfigcommand.hpp"
#include "cli/nodeutility.hpp"
#include "cli/repositoryutility.hpp"
#include "cli/variableutility.hpp"
#include "base/logger.hpp"
#include "base/console.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include "base/json.hpp"
#include "base/objectlock.hpp"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("node/update-config", NodeUpdateConfigCommand);

String NodeUpdateConfigCommand::GetDescription(void) const
{
	return "Update Icinga 2 node config.";
}

String NodeUpdateConfigCommand::GetShortDescription(void) const
{
	return "update node config";
}

ImpersonationLevel NodeUpdateConfigCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

bool NodeUpdateConfigCommand::IsDeprecated(void) const
{
	return true;
}

/**
 * The entry point for the "node update-config" CLI command.
 *
 * @returns An exit status.
 */
int NodeUpdateConfigCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	//If there are changes pending, abort the current operation
	if (RepositoryUtility::ChangeLogHasPendingChanges()) {
		Log(LogWarning, "cli")
		    << "There are pending changes for commit.\n"
		    << "Please review and commit them using 'icinga2 repository commit [--simulate]'\n"
		    << "or drop them using 'icinga2 repository clear-changes' before proceeding.";
		return 1;
	}

	String inventory_path = NodeUtility::GetRepositoryPath() + "/inventory.index";

	Dictionary::Ptr old_inventory = new Dictionary();
	if (Utility::PathExists(inventory_path)) {
		old_inventory = Utility::LoadJsonFile(inventory_path);
	}

	Dictionary::Ptr inventory = new Dictionary();

	Log(LogInformation, "cli")
	    << "Updating node configuration for ";

	NodeUtility::PrintNodes(std::cout);

	/* cache all existing object configs only once and pass it to AddObject() */
	std::vector<String> object_paths = RepositoryUtility::GetObjects();
	/* cache all existing changes only once and pass it to AddObject() */
	Array::Ptr changes = new Array();
	RepositoryUtility::GetChangeLog(boost::bind(RepositoryUtility::CollectChange, _1, changes));

	std::vector<Dictionary::Ptr> nodes = NodeUtility::GetNodes();

	/* first make sure that all nodes are valid and should not be removed */
	for (const Dictionary::Ptr& node : nodes) {
		Dictionary::Ptr repository = node->Get("repository");
		String zone = node->Get("zone");
		String endpoint = node->Get("endpoint");
		String node_name = endpoint;

		/* store existing structure in index */
		inventory->Set(endpoint, node);
	}

	if (old_inventory) {
		/* check if there are objects inside the old_inventory which do not exist anymore */
		ObjectLock ulock(old_inventory);
		for (const Dictionary::Pair& old_node_objs : old_inventory) {

			String old_node_name = old_node_objs.first;

			/* check if the node was dropped */
			if (!inventory->Contains(old_node_name)) {
				Log(LogInformation, "cli")
				    << "Node update found old node '" << old_node_name << "'. Removing it and all of its hosts/services.";

				//TODO Remove an node and all of his hosts
				Dictionary::Ptr old_node = old_node_objs.second;
				Dictionary::Ptr old_node_repository = old_node->Get("repository");

				if (old_node_repository) {
					ObjectLock olock(old_node_repository);
					for (const Dictionary::Pair& kv : old_node_repository) {
						String host = kv.first;

						Dictionary::Ptr host_attrs = new Dictionary();
						host_attrs->Set("name", host);
						RepositoryUtility::RemoveObject(host, "Host", host_attrs, changes); //this removes all services for this host as well
					}
				}

				String zone = old_node->Get("zone");
				String endpoint = old_node->Get("endpoint");

				Dictionary::Ptr zone_attrs = new Dictionary();
				zone_attrs->Set("name", zone);
				RepositoryUtility::RemoveObject(zone, "Zone", zone_attrs, changes);

				Dictionary::Ptr endpoint_attrs = new Dictionary();
				endpoint_attrs->Set("name", endpoint);
				RepositoryUtility::RemoveObject(endpoint, "Endpoint", endpoint_attrs, changes);
			} else {
				/* get the current node */
				Dictionary::Ptr new_node = inventory->Get(old_node_name);
				Dictionary::Ptr new_node_repository = new_node->Get("repository");

				Dictionary::Ptr old_node = old_node_objs.second;
				Dictionary::Ptr old_node_repository = old_node->Get("repository");

				if (old_node_repository) {
					ObjectLock xlock(old_node_repository);
					for (const Dictionary::Pair& kv : old_node_repository) {
						String old_host = kv.first;

						if (old_host == "localhost") {
							Log(LogWarning, "cli")
							    << "Ignoring host '" << old_host << "'. Please make sure to configure a unique name on your node '" << old_node_name << "'.";
							continue;
						}

						/* check against black/whitelist before trying to remove host */
						if (NodeUtility::CheckAgainstBlackAndWhiteList("blacklist", old_node_name, old_host, Empty) &&
						    !NodeUtility::CheckAgainstBlackAndWhiteList("whitelist", old_node_name, old_host, Empty)) {
							Log(LogWarning, "cli")
							    << "Host '" << old_node_name << "' on node '" << old_node_name << "' is blacklisted, but not whitelisted. Skipping.";
							continue;
						}

						if (!new_node_repository->Contains(old_host)) {
							Log(LogInformation, "cli")
							    << "Node update found old host '" << old_host << "' on node '" << old_node_name << "'. Removing it.";

							Dictionary::Ptr host_attrs = new Dictionary();
							host_attrs->Set("name", old_host);
							RepositoryUtility::RemoveObject(old_host, "Host", host_attrs, changes); //this will remove all services for this host too
						} else {
							/* host exists, now check all services for this host */
							Array::Ptr old_services = kv.second;
							Array::Ptr new_services = new_node_repository->Get(old_host);

							ObjectLock ylock(old_services);
							for (const String& old_service : old_services) {
								/* check against black/whitelist before trying to remove service */
								if (NodeUtility::CheckAgainstBlackAndWhiteList("blacklist", old_node_name, old_host, old_service) &&
								    !NodeUtility::CheckAgainstBlackAndWhiteList("whitelist", old_node_name, old_host, old_service)) {
									Log(LogWarning, "cli")
									    << "Service '" << old_service << "' on host '" << old_host << "' on node '"
									    << old_node_name << "' is blacklisted, but not whitelisted. Skipping.";
									continue;
								}

								if (!new_services->Contains(old_service)) {
									Log(LogInformation, "cli")
									    << "Node update found old service '" << old_service << "' on host '" << old_host
									    << "' on node '" << old_node_name << "'. Removing it.";

									Dictionary::Ptr service_attrs = new Dictionary();
									service_attrs->Set("name", old_service);
									service_attrs->Set("host_name", old_host);
									RepositoryUtility::RemoveObject(old_service, "Service", service_attrs, changes);
								}
							}
						}
					}
				}
			}
		}
	}

	/* next iterate over all nodes and add hosts/services */
	for (const Dictionary::Ptr& node : nodes) {
		Dictionary::Ptr repository = node->Get("repository");
		String zone = node->Get("zone");
		String endpoint = node->Get("endpoint");
		String node_name = endpoint;

		Dictionary::Ptr host_services = new Dictionary();

		if (NodeUtility::CheckAgainstBlackAndWhiteList("blacklist", node_name, "*", Empty) &&
		    !NodeUtility::CheckAgainstBlackAndWhiteList("whitelist", node_name, "*", Empty)) {
			Log(LogWarning, "cli")
			    << "Skipping node '" << node_name << "' on blacklist.";
			continue;
		}

		Log(LogInformation, "cli")
		    << "Adding host '" << zone << "' to the repository.";

		Dictionary::Ptr host_attrs = new Dictionary();
		host_attrs->Set("__name", zone);
		host_attrs->Set("name", zone);
		host_attrs->Set("check_command", "cluster-zone");
		Array::Ptr host_imports = new Array();
		host_imports->Add("satellite-host"); //default host node template
		host_attrs->Set("import", host_imports);

		if (!RepositoryUtility::AddObject(object_paths, zone, "Host", host_attrs, changes, false)) {
			Log(LogWarning, "cli")
			    << "Cannot add node host '" << zone << "' to the config repository!\n";
		}

		if (repository) {
			ObjectLock olock(repository);
			for (const Dictionary::Pair& kv : repository) {
				String host = kv.first;
				String host_pattern = host + ".conf";
				bool skip_host = false;

				if (host == "localhost") {
					Log(LogWarning, "cli")
					    << "Ignoring host '" << host << "'. Please make sure to configure a unique name on your node '" << endpoint << "'.";
					continue;
				}

				for (const String& object_path : object_paths) {
					if (object_path.Contains(host_pattern)) {
						Log(LogNotice, "cli")
						    << "Host '" << host << "' already exists.";
						skip_host = true;
						break;
					}
				}

				/* host has already been created above */
				if (host == zone)
					skip_host = true;

				bool host_was_blacklisted = false;

				/* check against black/whitelist before trying to add host */
				if (NodeUtility::CheckAgainstBlackAndWhiteList("blacklist", node_name, host, Empty) &&
				    !NodeUtility::CheckAgainstBlackAndWhiteList("whitelist", node_name, host, Empty)) {
					Log(LogWarning, "cli")
					    << "Host '" << host << "' on node '" << node_name << "' is blacklisted, but not whitelisted. Not creating host object.";
					skip_host = true;
					host_was_blacklisted = true; //check this for services on this blacklisted host
				}

				if (!skip_host) {
					/* add a new host to the config repository */
					Dictionary::Ptr host_attrs = new Dictionary();
					host_attrs->Set("__name", host);
					host_attrs->Set("name", host);

					if (host == zone)
						host_attrs->Set("check_command", "cluster-zone");
					else {
						host_attrs->Set("check_command", "dummy");
						host_attrs->Set("zone", zone);
					}

					Array::Ptr host_imports = new Array();
					host_imports->Add("satellite-host"); //default host node template
					host_attrs->Set("import", host_imports);

					RepositoryUtility::AddObject(object_paths, host, "Host", host_attrs, changes, false);
				}

				/* special condition: what if the host was blacklisted before, but the services should be generated? */
				if (host_was_blacklisted) {
					Log(LogNotice, "cli")
					    << "Host '" << host << "' was blacklisted. Won't generate any services.";
					continue;
				}

				Array::Ptr services = kv.second;

				if (services->GetLength() == 0) {
					Log(LogNotice, "cli")
					    << "Host '" << host << "' without services.";
					continue;
				}

				ObjectLock xlock(services);
				for (const String& service : services) {
					bool skip_service = false;

					String service_pattern = host + "/" + service + ".conf";

					for (const String& object_path : object_paths) {
						if (object_path.Contains(service_pattern)) {
							Log(LogNotice, "cli")
							    << "Service '" << service << "' on Host '" << host << "' already exists.";
							skip_service = true;
							break;
						}
					}

					/* check against black/whitelist before trying to add service */
					if (NodeUtility::CheckAgainstBlackAndWhiteList("blacklist", endpoint, host, service) &&
					    !NodeUtility::CheckAgainstBlackAndWhiteList("whitelist", endpoint, host, service)) {
						Log(LogWarning, "cli")
						    << "Service '" << service << "' on host '" << host << "' on node '"
						    << node_name << "' is blacklisted, but not whitelisted. Not creating service object.";
						skip_service = true;
					}

					if (skip_service)
						continue;

					/* add a new service for this host to the config repository */
					Dictionary::Ptr service_attrs = new Dictionary();
					String long_name = host + "!" + service; //use NameComposer?
					service_attrs->Set("__name", long_name);
					service_attrs->Set("name", service);
					service_attrs->Set("host_name", host); //Required for host-service relation
					service_attrs->Set("check_command", "dummy");
					service_attrs->Set("zone", zone);

					Array::Ptr service_imports = new Array();
					service_imports->Add("satellite-service"); //default service node template
					service_attrs->Set("import", service_imports);

					if (!RepositoryUtility::AddObject(object_paths, service, "Service", service_attrs, changes, false))
						continue;
				}
			}
		}

		/* write a new zone and endpoint for the node */
		Dictionary::Ptr endpoint_attrs = new Dictionary();
		endpoint_attrs->Set("__name", endpoint);
		endpoint_attrs->Set("name", endpoint);

		Dictionary::Ptr settings = node->Get("settings");

		if (settings) {
			if (settings->Contains("host"))
				endpoint_attrs->Set("host", settings->Get("host"));
			if (settings->Contains("port"))
				endpoint_attrs->Set("port", settings->Get("port"));
		}

		Log(LogInformation, "cli")
		    << "Adding endpoint '" << endpoint << "' to the repository.";

		if (!RepositoryUtility::AddObject(object_paths, endpoint, "Endpoint", endpoint_attrs, changes, false)) {
			Log(LogWarning, "cli")
			    << "Cannot add node endpoint '" << endpoint << "' to the config repository!\n";
		}

		Dictionary::Ptr zone_attrs = new Dictionary();
		Array::Ptr zone_members = new Array();

		zone_members->Add(endpoint);
		zone_attrs->Set("__name", zone);
		zone_attrs->Set("name", zone);
		zone_attrs->Set("endpoints", zone_members);

		String parent_zone = VariableUtility::GetVariable("ZoneName");

		if (parent_zone.IsEmpty()) {
			Log(LogWarning, "cli")
				<< "Variable 'ZoneName' is not set. Falling back to using 'master' as default. Please verify the generated configuration.";
			parent_zone = "master";
		}

		zone_attrs->Set("parent", parent_zone);

		Log(LogInformation, "cli")
		    << "Adding zone '" << zone << "' to the repository.";

		if (!RepositoryUtility::AddObject(object_paths, zone, "Zone", zone_attrs, changes, false)) {
			Log(LogWarning, "cli")
			    << "Cannot add node zone '" << zone << "' to the config repository!\n";
		}
	}

	Log(LogInformation, "cli", "Committing node configuration.");

	RepositoryUtility::PrintChangeLog(std::cout);
	std::cout << "\n";
	RepositoryUtility::CommitChangeLog();

	/* store the new inventory for next run */
	NodeUtility::CreateRepositoryPath();
	Utility::SaveJsonFile(inventory_path, 0600, inventory);

	std::cout << "Make sure to reload Icinga 2 for these changes to take effect." << std::endl;

	return 0;
}
