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

#include "remote/configmoduleshandler.hpp"
#include "remote/configmoduleutility.hpp"
#include "remote/httputility.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/modules", ConfigModulesHandler);

void ConfigModulesHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod == "GET")
		HandleGet(user, request, response);
	else if (request.RequestMethod == "POST")
		HandlePost(user, request, response);
	else if (request.RequestMethod == "DELETE")
		HandleDelete(user, request, response);
	else
		response.SetStatus(400, "Bad request");
}

bool ConfigModulesHandler::CanAlsoHandleUrl(const Url::Ptr& url) const
{
	if (url->GetPath().size() > 4)
		return false;
	else
		return true;
}

void ConfigModulesHandler::HandleGet(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	std::vector<String> modules = ConfigModuleUtility::GetModules();

	Array::Ptr results = new Array();

	BOOST_FOREACH(const String& module, modules) {
		Dictionary::Ptr moduleInfo = new Dictionary();
		moduleInfo->Set("name", module);
		moduleInfo->Set("stages", Array::FromVector(ConfigModuleUtility::GetStages(module)));
		moduleInfo->Set("active-stage", ConfigModuleUtility::GetActiveStage(module));
		results->Add(moduleInfo);
	}

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(200, "OK");
	HttpUtility::SendJsonBody(response, result);
}

void ConfigModulesHandler::HandlePost(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("module", request.RequestUrl->GetPath()[3]);

	String moduleName = params->Get("module");

	if (!ConfigModuleUtility::ValidateName(moduleName)) {
		response.SetStatus(403, "Forbidden");
		return;
	}

	int code = 200;
	String status = "Created module.";

	try {
		ConfigModuleUtility::CreateModule(moduleName);
	} catch (const std::exception& ex) {
		code = 501;
		status = "Error: " + DiagnosticInformation(ex);
	}

	Dictionary::Ptr result1 = new Dictionary();

	result1->Set("module", moduleName);
	result1->Set("code", code);
	result1->Set("status", status);

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(code, (code == 200) ? "OK" : "Error");
	HttpUtility::SendJsonBody(response, result);
}

void ConfigModulesHandler::HandleDelete(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	if (request.RequestUrl->GetPath().size() >= 4)
		params->Set("module", request.RequestUrl->GetPath()[3]);

	String moduleName = params->Get("module");

	if (!ConfigModuleUtility::ValidateName(moduleName)) {
		response.SetStatus(403, "Forbidden");
		return;
	}

	int code = 200;
	String status = "Deleted module.";

	try {
		ConfigModuleUtility::DeleteModule(moduleName);
	} catch (const std::exception& ex) {
		code = 501;
		status = "Error: " + DiagnosticInformation(ex);
	}

	Dictionary::Ptr result1 = new Dictionary();

	result1->Set("module", moduleName);
	result1->Set("code", code);
	result1->Set("status", status);

	Array::Ptr results = new Array();
	results->Add(result1);

	Dictionary::Ptr result = new Dictionary();
	result->Set("results", results);

	response.SetStatus(code, (code == 200) ? "OK" : "Error");
	HttpUtility::SendJsonBody(response, result);
}

