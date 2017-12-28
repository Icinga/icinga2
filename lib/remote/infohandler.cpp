/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/infohandler.hpp"
#include "remote/httputility.hpp"
#include "base/application.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/", InfoHandler);

bool InfoHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestUrl->GetPath().size() > 2)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	if (request.RequestUrl->GetPath().empty()) {
		response.SetStatus(302, "Found");
		response.AddHeader("Location", "/v1");
		return true;
	}

	if (request.RequestUrl->GetPath()[0] != "v1" || request.RequestUrl->GetPath().size() != 1)
		return false;

	response.SetStatus(200, "OK");

	std::vector<String> permInfo;
	Array::Ptr permissions = user->GetPermissions();

	if (permissions) {
		ObjectLock olock(permissions);
		for (const Value& permission : permissions) {
			std::ostringstream nameBuf;
			bool hasFilter = false;
			if (permission.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dpermission = permission;
				nameBuf << dpermission->Get("permission");
				hasFilter = dpermission->Contains("filter");
			} else
				nameBuf << permission;

			if (hasFilter)
				nameBuf << " (filtered)";

			permInfo.emplace_back(nameBuf.str());
		}
	}

	if (request.Headers->Get("accept") == "application/json") {
		Dictionary::Ptr result1 = new Dictionary();

		result1->Set("user", user->GetName());
		result1->Set("permissions", Array::FromVector(permInfo));
		result1->Set("version", Application::GetAppVersion());
		result1->Set("info", "More information about API requests is available in the documentation at https://docs.icinga.com/icinga2/latest.");

		Array::Ptr results = new Array();
		results->Add(result1);

		Dictionary::Ptr result = new Dictionary();
		result->Set("results", results);

		HttpUtility::SendJsonBody(response, result);
	} else {
		response.AddHeader("Content-Type", "text/html");

		std::ostringstream bodyBuf;
		bodyBuf << "<html><head><title>Icinga 2</title></head><h1>Hello from Icinga 2 (Version: " << Application::GetAppVersion() << ")!</h1>";
		bodyBuf << "<p>You are authenticated as <b>" << user->GetName() << "</b>. ";

		if (!permInfo.empty()) {
			bodyBuf << "Your user has the following permissions:</p> <ul>";

			for (const String& perm : permInfo)
				bodyBuf << "<li>" << perm << "</li>";

			bodyBuf << "</ul>";
		} else
			bodyBuf << "Your user does not have any permissions.</p>";

		bodyBuf << "<p>More information about API requests is available in the <a href=\"https://docs.icinga.com/icinga2/latest\" target=\"_blank\">documentation</a>.</p></html>";

		String body = bodyBuf.str();
		response.WriteBody(body.CStr(), body.GetLength());
	}

	return true;
}

