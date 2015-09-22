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

#include "remote/configpackageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/packages", ConfigPackagesHandler);

bool ConfigPackagesHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestUrl->GetPath().size() > 4) {
		String path = boost::algorithm::join(request.RequestUrl->GetPath(), "/");
		HttpUtility::SendJsonError(response, 404, "The requested path is too long to match any config package requests");
		return true;
	}

	if (request.RequestMethod == "GET")
		HandleGet(user, request, response);
	else if (request.RequestMethod == "POST")
		HandlePost(user, request, response);
	else if (request.RequestMethod == "DELETE")
		HandleDelete(user, request, response);
	else
		HttpUtility::SendJsonError(response, 400, "Invalid request type. Must be GET, POST or DELETE.");

	return true;
}

void ConfigPackagesHandler::HandleGet(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	std::vector<String> packages = ConfigPackageUtility::GetPackages();

	Array::Ptr results = new Array();

	BOOST_FOREACH(const String& package, packages) {
		Dictionary::Ptr packageInfo = new Dictionary();
		packageInfo->Set("name", package);
		packageInfo->Set("stages", Array::FromVector(ConfigPackageUtility::GetStages(package)));
		packageInfo->Set("active-stage", ConfigPackageUtility::GetActiveStage(package));
		results->Add(packageInfo);
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);
}

void ConfigPackagesHandler::HandlePost(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, 404, "Package is not valid or does not exist.");
		return;
	}

	Dictionary::Ptr result1 = new Dictionary();

	try {
		ConfigPackageUtility::CreatePackage(packageName);
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, 500, "Could not create package.",
			request.GetVerboseErrors() ? DiagnosticInformation(ex) : "");
	}

	result1->Set("code", 200);
	result1->Set("status", "Created package.");

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);
}

void ConfigPackagesHandler::HandleDelete(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, 404, "Package is not valid or does not exist.");
		return;
	}

	int code = 200;
	String status = "Deleted package.";
	Dictionary::Ptr result1 = new Dictionary();

	try {
		ConfigPackageUtility::DeletePackage(packageName);
	} catch (const std::exception& ex) {
		code = 500;
		status = "Failed to delete package.";
		if (request.GetVerboseErrors())
			result1->Set("diagnostic information", DiagnosticInformation(ex));
	}


	result1->Set("package", packageName);
	result1->Set("code", code);
	result1->Set("status", status);

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(code, (code == 200) ? "OK" : "Internal Server Error");
	HttpUtility::SendJsonBody(response, result);
}

