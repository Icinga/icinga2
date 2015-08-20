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

#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/jsonrpc.hpp"
#include "base/configtype.hpp"
#include "base/json.hpp"
#include <fstream>

using namespace icinga;

INITIALIZE_ONCE(&ApiListener::StaticInitialize);

REGISTER_APIFUNCTION(UpdateObject, config, &ApiListener::ConfigUpdateObjectAPIHandler);
REGISTER_APIFUNCTION(DeleteObject, config, &ApiListener::ConfigDeleteObjectAPIHandler);


void ApiListener::StaticInitialize(void)
{
	ConfigObject::OnActiveChanged.connect(&ApiListener::ConfigUpdateObjectHandler);
	ConfigObject::OnVersionChanged.connect(&ApiListener::ConfigUpdateObjectHandler);
}

void ApiListener::ConfigUpdateObjectHandler(const ConfigObject::Ptr& object, const Value& cookie)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	if (object->IsActive()) {
		/* Sync object config */
		listener->UpdateConfigObject(object, cookie);
	} else {
		/* Delete object */
	}
}

Value ApiListener::ConfigUpdateObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Log(LogWarning, "ApiListener")
	    << "Received update for object: " << JsonEncode(params);

	ConfigType::Ptr dtype = ConfigType::GetByName(params->Get("type"));

	if (!dtype) {
		Log(LogCritical, "ApiListener")
		    << "Config type '" << params->Get("type") << "' does not exist.";
		return Empty;
	}

	ConfigObject::Ptr object = dtype->GetObject(params->Get("name"));

	if (!object) {
		Array::Ptr errors = new Array();
		if (!ConfigObjectUtility::CreateObject(Type::GetByName(params->Get("type")),
		    params->Get("name"), params->Get("config"), errors)) {
		    	Log(LogCritical, "ApiListener", "Could not create object:");

		    	ObjectLock olock(errors);
		    	BOOST_FOREACH(const String& error, errors) {
		    		Log(LogCritical, "ApiListener", error);
		    	}
		}

		//TODO-MA: modified attributes, same version
	}

	return Empty;
}

Value ApiListener::ConfigDeleteObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	return Empty;
}

void ApiListener::UpdateConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
    const JsonRpcConnection::Ptr& client)
{
	if (object->GetPackage() != "_api")
		return;

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::UpdateObject");

	Dictionary::Ptr params = new Dictionary();
	params->Set("name", object->GetName());
	params->Set("type", object->GetType()->GetName());
	params->Set("version", object->GetVersion());

	String file = ConfigObjectUtility::GetObjectConfigPath(object->GetReflectionType(), object->GetName());

	std::ifstream fp(file.CStr(), std::ifstream::binary);
	if (!fp)
		return;

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
	params->Set("config", content);

	Dictionary::Ptr original_attributes = object->GetOriginalAttributes();
	Dictionary::Ptr modified_attributes = new Dictionary();

	if (original_attributes) {
		ObjectLock olock(original_attributes);
		BOOST_FOREACH(const Dictionary::Pair& kv, original_attributes) {
			int fid = object->GetReflectionType()->GetFieldId(kv.first);
			//TODO-MA: vars.os
			Value value = static_cast<Object::Ptr>(object)->GetField(fid);
			modified_attributes->Set(kv.first, value);
		}
	}

	params->Set("modified_attributes", modified_attributes);

	message->Set("params", params);

	Log(LogWarning, "ApiListener")
	    << "Sent update for object: " << JsonEncode(params);

	if (client)
		JsonRpc::SendMessage(client->GetStream(), message);
	else
		RelayMessage(origin, object, message, false);
}
