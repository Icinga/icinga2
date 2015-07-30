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

#include "remote/httputility.hpp"
#include "base/json.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

Dictionary::Ptr HttpUtility::FetchRequestParameters(HttpRequest& request)
{
	Dictionary::Ptr result;

	String body;
	char buffer[1024];
	size_t count;

	while ((count = request.ReadBody(buffer, sizeof(buffer))) > 0)
		body += String(buffer, buffer + count);

	if (!body.IsEmpty())
		result = JsonDecode(body);

	if (!result)
		result = new Dictionary();

	typedef std::pair<String, std::vector<String> > kv_pair;
	BOOST_FOREACH(const kv_pair& kv, request.RequestUrl->GetQuery()) {
		result->Set(kv.first, Array::FromVector(kv.second));
	}

	return result;
}

void HttpUtility::SendJsonBody(HttpResponse& response, const Value& val)
{
	response.AddHeader("Content-Type", "application/json");

	String body = JsonEncode(val);
	response.WriteBody(body.CStr(), body.GetLength());
}

Value HttpUtility::GetLastParameter(const Dictionary::Ptr& params, const String& key)
{
	Value varr = params->Get(key);

	if (!varr.IsObjectType<Array>())
		return varr;

	Array::Ptr arr = varr;

	if (arr->GetLength() == 0)
		return Empty;
	else
		return arr->Get(arr->GetLength() - 1);
}
