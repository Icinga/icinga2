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

#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include "remote/httprequest.hpp"
#include "remote/httpresponse.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * Helper functions.
 *
 * @ingroup remote
 */
class HttpUtility
{

public:
	static Dictionary::Ptr FetchRequestParameters(HttpRequest& request);
	static void SendJsonBody(HttpResponse& response, const Dictionary::Ptr& params, const Value& val);
	static Value GetLastParameter(const Dictionary::Ptr& params, const String& key);
	static void SendJsonError(HttpResponse& response, const Dictionary::Ptr& params, const int code,
		const String& verbose = String(), const String& diagnosticInformation = String());

private:
	static String GetErrorNameByCode(int code);

};

}

#endif /* HTTPUTILITY_H */
