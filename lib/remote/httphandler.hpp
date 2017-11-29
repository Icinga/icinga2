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

#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include "remote/i2-remote.hpp"
#include "remote/httpresponse.hpp"
#include "remote/apiuser.hpp"
#include "base/registry.hpp"
#include <vector>

namespace icinga
{

/**
 * HTTP handler.
 *
 * @ingroup remote
 */
class I2_REMOTE_API HttpHandler : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(HttpHandler);

	virtual bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response, const Dictionary::Ptr& params) = 0;

	static void Register(const Url::Ptr& url, const HttpHandler::Ptr& handler);
	static void ProcessRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response);

private:
	static Dictionary::Ptr m_UrlTree;
};

/**
 * Helper class for registering HTTP handlers.
 *
 * @ingroup remote
 */
class I2_REMOTE_API RegisterHttpHandler
{
public:
	RegisterHttpHandler(const String& url, const HttpHandler& function);
};

#define REGISTER_URLHANDLER(url, klass) \
	INITIALIZE_ONCE([]() { \
		Url::Ptr uurl = new Url(url); \
		HttpHandler::Ptr handler = new klass(); \
		HttpHandler::Register(uurl, handler); \
	})

}

#endif /* HTTPHANDLER_H */
