/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "remote/configpackageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/packages", ConfigPackagesHandler);

bool ConfigPackagesHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() > 4)
		return false;

	if (request.RequestMethod == "GET")
		HandleGet(user, request, response, params);
	else if (request.RequestMethod == "POST")
		HandlePost(user, request, response, params);
	else if (request.RequestMethod == "DELETE")
		HandleDelete(user, request, response, params);
	else
		return false;

	return true;
}

void ConfigPackagesHandler::HandleGet(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	FilterUtility::CheckPermission(user, "config/query");

	std::vector<String> packages;

	try {
		packages = ConfigPackageUtility::GetPackages();
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Could not retrieve packages.",
			DiagnosticInformation(ex));
		return;
	}

	ArrayData results;

	{
		boost::mutex::scoped_lock lock(ConfigPackageUtility::GetStaticMutex());
		for (const String& package : packages) {
			results.emplace_back(new Dictionary({
				{ "name", package },
				{ "stages", Array::FromVector(ConfigPackageUtility::GetStages(package)) },
				{ "active-stage", ConfigPackageUtility::GetActiveStage(package) }
			}));
		}
	}

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array(std::move(results)) }
	});

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigPackagesHandler::HandlePost(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	FilterUtility::CheckPermission(user, "config/modify");

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	try {
		boost::mutex::scoped_lock lock(ConfigPackageUtility::GetStaticMutex());
		ConfigPackageUtility::CreatePackage(packageName);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Could not create package '" + packageName + "'.",
			DiagnosticInformation(ex));
		return;
	}

	Dictionary::Ptr result1 = new Dictionary({
		{ "code", 200 },
		{ "package", packageName },
		{ "status", "Created package." }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);
}

void ConfigPackagesHandler::HandleDelete(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	FilterUtility::CheckPermission(user, "config/modify");

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name '" + packageName + "'.");
		return;
	}

	try {
		ConfigPackageUtility::DeletePackage(packageName);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Failed to delete package '" + packageName + "'.",
			DiagnosticInformation(ex));
		return;
	}

	Dictionary::Ptr result1 = new Dictionary({
		{ "code", 200 },
		{ "package", packageName },
		{ "status", "Deleted package." }
	});

	Dictionary::Ptr result = new Dictionary({
		{ "results", new Array({ result1 }) }
	});

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, params, result);
}
