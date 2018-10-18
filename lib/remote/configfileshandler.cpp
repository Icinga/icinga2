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

#include "remote/configfileshandler.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/httputility.hpp"
#include "remote/filterutility.hpp"
#include "base/exception.hpp"
#include <boost/algorithm/string/join.hpp>
#include <fstream>

using namespace icinga;

REGISTER_URLHANDLER("/v1/config/files", ConfigFilesHandler);

bool ConfigFilesHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params)
{
	if (request.RequestMethod != "GET")
		return false;

	const std::vector<String>& urlPath = request.RequestUrl->GetPath();

	if (urlPath.size() >= 4)
		params->Set("package", urlPath[3]);

	if (urlPath.size() >= 5)
		params->Set("stage", urlPath[4]);

	if (urlPath.size() >= 6) {
		std::vector<String> tmpPath(urlPath.begin() + 5, urlPath.end());
		params->Set("path", boost::algorithm::join(tmpPath, "/"));
	}

	if (request.Headers->Get("accept") == "application/json") {
		HttpUtility::SendJsonError(response, params, 400, "Invalid Accept header. Either remove the Accept header or set it to 'application/octet-stream'.");
		return true;
	}

	FilterUtility::CheckPermission(user, "config/query");

	String packageName = HttpUtility::GetLastParameter(params, "package");
	String stageName = HttpUtility::GetLastParameter(params, "stage");

	if (!ConfigPackageUtility::ValidateName(packageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid package name.");
		return true;
	}

	if (!ConfigPackageUtility::ValidateName(stageName)) {
		HttpUtility::SendJsonError(response, params, 400, "Invalid stage name.");
		return true;
	}

	String relativePath = HttpUtility::GetLastParameter(params, "path");

	if (ConfigPackageUtility::ContainsDotDot(relativePath)) {
		HttpUtility::SendJsonError(response, params, 400, "Path contains '..' (not allowed).");
		return true;
	}

	String path = ConfigPackageUtility::GetPackageDir() + "/" + packageName + "/" + stageName + "/" + relativePath;

	if (!Utility::PathExists(path)) {
		HttpUtility::SendJsonError(response, params, 404, "Path not found.");
		return true;
	}

	try {
		std::ifstream fp(path.CStr(), std::ifstream::in | std::ifstream::binary);
		fp.exceptions(std::ifstream::badbit);

		String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
		response.SetStatus(200, "OK");
		response.AddHeader("Content-Type", "application/octet-stream");
		response.WriteBody(content.CStr(), content.GetLength());
	} catch (const std::exception& ex) {
		HttpUtility::SendJsonError(response, params, 500, "Could not read file.",
			DiagnosticInformation(ex));
	}

	return true;
}
