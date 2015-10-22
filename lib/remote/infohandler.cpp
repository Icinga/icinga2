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

#include "remote/infohandler.hpp"
#include "remote/httputility.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/v1", InfoHandler);

bool InfoHandler::HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response)
{
	if (request.RequestUrl->GetPath().size() != 1)
		return false;

	if (request.RequestMethod != "GET")
		return false;

	response.SetStatus(200, "OK");
	response.AddHeader("Content-Type", "text/html");

	String body = "<html><head><title>Icinga 2</title></head><h1>Hello from Icinga 2!</h1>";
	body += "<p>You are authenticated as <b>" + user->GetName() + "</b>. ";

	bool has_permissions = false;
	String perm_info;

	Array::Ptr permissions = user->GetPermissions();
	if (permissions) {
		ObjectLock olock(permissions);
		BOOST_FOREACH(const Value& permission, permissions) {
			has_permissions = true;

			String name;
			bool has_filter = false;
			if (permission.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dpermission = permission;
				name = dpermission->Get("permission");
				has_filter = dpermission->Contains("filter");
			} else
				name = permission;

			perm_info += "<li>" + name;
			if (has_filter)
				perm_info += " (filtered)";
			perm_info += "</li>";
		}
	}

	if (has_permissions)
		body += "Your user has the following permissions:</p> <ul>" + perm_info + "</ul>";
	else
		body += "Your user does not have any permissions.</p>";

	body += "<p>More information about API requests is available in the documentation.</p></html>";
	response.WriteBody(body.CStr(), body.GetLength());

	return true;
}

