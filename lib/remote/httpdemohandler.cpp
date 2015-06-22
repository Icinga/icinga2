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

#include "remote/httpdemohandler.hpp"

using namespace icinga;

REGISTER_URLHANDLER("/demo", HttpDemoHandler);

void HttpDemoHandler::HandleRequest(HttpRequest& request, HttpResponse& response)
{
	if (request.RequestMethod == "GET") {
		String form = "<form action=\"/demo\" method=\"post\"><input type=\"text\" name=\"msg\"><input type=\"submit\"></form>";
		response.SetStatus(200, "OK");
		response.AddHeader("Content-Type", "text/html");
		response.WriteBody(form.CStr(), form.GetLength());
	} else if (request.RequestMethod == "POST") {
		response.SetStatus(200, "OK");
		String msg = "You sent: ";

		char buffer[512];
		size_t count;
		while ((count = request.ReadBody(buffer, sizeof(buffer))) > 0)
			msg += String(buffer, buffer + count);
		response.WriteBody(msg.CStr(), msg.GetLength());
	} else {
		response.SetStatus(400, "Bad request");
	}
}

