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

#include "remote/configfileshandler.hpp"
#include "remote/configmoduleutility.hpp"
#include "remote/httputility.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>
#include <fstream>

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/files", ConfigFilesHandler);

void ConfigFilesHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod == "GET")
		HandleGet(user, request, response);
	else
		response.SetStatus(400, "Bad request");
}

bool ConfigFilesHandler::CanAlsoHandleUrl(const Url::Ptr& url) const
{
	return true;
}

void ConfigFilesHandler::HandleGet(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	Dictionary::Ptr params = HttpUtility::FetchRequestParameters(request);

	const std::vector<String>& urlPath = request.RequestUrl->GetPath();

	if (urlPath.size() >= 4)
		params->Set("module", urlPath[3]);

	if (urlPath.size() >= 5)
		params->Set("stage", urlPath[4]);

	if (urlPath.size() >= 6) {
		std::vector<String> tmpPath(urlPath.begin() + 5, urlPath.end());
		params->Set("path", boost::algorithm::join(tmpPath, "/"));
	}

	String moduleName = params->Get("module");
	String stageName = params->Get("stage");

	if (!ConfigModuleUtility::ValidateName(moduleName) || !ConfigModuleUtility::ValidateName(stageName)) {
		response.SetStatus(403, "Forbidden");
		return;
	}

	String relativePath = params->Get("path");

	if (ConfigModuleUtility::ContainsDotDot(relativePath)) {
		response.SetStatus(403, "Forbidden");
		return;
	}

	String path = ConfigModuleUtility::GetModuleDir() + "/" + moduleName + "/" + stageName + "/" + relativePath;

	if (!Utility::PathExists(path)) {
		response.SetStatus(404, "File not found");
		return;
	}

	try {
		std::ifstream fp(path.CStr(), std::ifstream::in | std::ifstream::binary);
		fp.exceptions(std::ifstream::badbit);

		String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
		response.SetStatus(200, "OK");
		response.AddHeader("Content-Type", "application/octet-stream");
		response.WriteBody(content.CStr(), content.GetLength());
	} catch (const std::exception& ex) {
		response.SetStatus(503, "Could not read file");
	}
}


