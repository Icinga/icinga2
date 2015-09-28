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

#include "remote/configstageshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/stages", ConfigStagesHandler);

bool ConfigStagesHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestUrl->GetPath().size() > 5) {
		String path = boost::algorithm::join(request.RequestUrl->GetPath(), "/");
		HttpUtility::SendJsonError(response, 404, "The requested path is too long to match any config tag requests.");
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

void ConfigStagesHandler::HandleGet(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	FilterUtility::CheckPermission(user, "config/query");

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	if (request.RequestUrl->GetPath().size() >= 5)
		params->Set("stage", request.RequestUrl->GetPath()[4]);

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidateName(packageName))
		return HttpUtility::SendJsonError(response, 404, "Package is not valid or does not exist.");

	if (!ConfigPackageUtility::ValidateName(stageName))
		return HttpUtility::SendJsonError(response, 404, "Stage is not valid or does not exist.");

	Array::Ptr results = new Array();

	std::vector<std::pair<String, bool> > paths = ConfigPackageUtility::GetFiles(packageName, stageName);

	String prefixPath = ConfigPackageUtility::GetPackageDir() + "/" + packageName + "/" + stageName + "/";

	typedef std::pair<String, bool> kv_pair;
	BOOST_FOREACH(const kv_pair& kv, paths) {
		Dictionary::Ptr stageInfo = new Dictionary();
		stageInfo->Set("type", (kv.second ? "directory" : "file"));
		stageInfo->Set("name", kv.first.SubStr(prefixPath.GetLength()));
		results->Add(stageInfo);
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);
}

void ConfigStagesHandler::HandlePost(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	FilterUtility::CheckPermission(user, "config/modify");

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	String packageName = HttpUtility::GetLastParameter(params, "package");

	if (!ConfigPackageUtility::ValidateName(packageName))
		return HttpUtility::SendJsonError(response, 404, "Package is not valid or does not exist.");

	Dictionary::Ptr files = params->Get("files");

	String stageName;

	try {
		if (!files)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Parameter 'files' must be specified."));

		stageName = ConfigPackageUtility::CreateStage(packageName, files);

		/* validate the config. on success, activate stage and reload */
		ConfigPackageUtility::AsyncTryActivateStage(packageName, stageName);
	} catch (const std::exception& ex) {
		return HttpUtility::SendJsonError(response, 500,
				"Stage creation failed.",
				request.GetVerboseErrors() ? DiagnosticInformation(ex) : "");
	}

	Dictionary::Ptr result1 = new Dictionary();

	result1->Set("code", 200);
	result1->Set("status", "Created stage.");

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);
}

void ConfigStagesHandler::HandleDelete(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	FilterUtility::CheckPermission(user, "config/modify");

	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("package", request.RequestUrl->GetPath()[3]);

	if (request.RequestUrl->GetPath().size() >= 5)
		params->Set("stage", request.RequestUrl->GetPath()[4]);

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidateName(packageName))
		return HttpUtility::SendJsonError(response, 404, "Package is not valid or does not exist.");

	if (!ConfigPackageUtility::ValidateName(stageName))
		return HttpUtility::SendJsonError(response, 404, "Stage is not valid or does not exist.");

	try {
		ConfigPackageUtility::DeleteStage(packageName, stageName);
	} catch (const std::exception& ex) {
		return HttpUtility::SendJsonError(response, 500,
		    "Failed to delete stage.",
		    request.GetVerboseErrors() ? DiagnosticInformation(ex) : "");
	}

	Dictionary::Ptr result1 = new Dictionary();

	result1->Set("code", 200);
	result1->Set("status", "Stage deleted");

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);
}

