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
#include "base/convert.hpp"
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

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return;
	}

	if (object->IsActive()) {
		/* Sync object config */
		listener->UpdateConfigObject(object, cookie);
	} else if (!object->IsActive() && object->GetExtension("ConfigObjectDeleted")) {
		/* Delete object */
		listener->DeleteConfigObject(object, cookie);
	}
}

Value ApiListener::ConfigUpdateObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Log(LogNotice, "ApiListener")
	    << "Received update for object: " << JsonEncode(params);

	/* check permissions */
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return Empty;
	}

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
		    << "Ignoring config update. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiListener")
		    << "Discarding 'config update object' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	Zone::Ptr lzone = Zone::GetLocalZone();
	String objZoneName = params->Get("zone");
	Zone::Ptr objZone = Zone::GetByName(objZoneName);

	/* discard messages if a) not the target zone and b) object zone is not a child of the local zone */
	if (objZone && (lzone->GetName() != objZoneName && !objZone->IsChildOf(lzone))) {
		Log(LogNotice, "ApiListener")
		    << "Discarding 'config update object' message from '"
		    << origin->FromClient->GetIdentity() << "': Object with zone '" << objZoneName << "' not allowed in zone '"
		    << lzone->GetName() << "'.";
		return Empty;
	}

	/* update the object */
	String objType = params->Get("type");
	String objName = params->Get("name");
	int objVersion = Convert::ToLong(params->Get("version"));

	ConfigType::Ptr dtype = ConfigType::GetByName(objType);

	if (!dtype) {
		Log(LogCritical, "ApiListener")
		    << "Config type '" << objType << "' does not exist.";
		return Empty;
	}

	ConfigObject::Ptr object = dtype->GetObject(objName);

	if (!object) {
		/* object does not exist, create it through the API */
		Array::Ptr errors = new Array();
		if (!ConfigObjectUtility::CreateObject(Type::GetByName(objType),
		    objName, params->Get("config"), errors)) {
			Log(LogCritical, "ApiListener")
			    << "Could not create object '" << objName << "':";

		    	ObjectLock olock(errors);
		    	BOOST_FOREACH(const String& error, errors) {
		    		Log(LogCritical, "ApiListener", error);
		    	}
		} else {
			/* object was created, update its version to its origin */
			ConfigObject::Ptr newObj = dtype->GetObject(objName);
			if (newObj)
				newObj->SetVersion(objVersion);
		}
	} else {
		/* object exists, update its attributes if version was changed */
		if (objVersion > object->GetVersion()) {
			Log(LogInformation, "ApiListener")
			    << "Processing config update for object '" << object->GetName()
			    << "': Object version " << object->GetVersion()
			    << " is older than the received version " << objVersion << ".";

			Dictionary::Ptr modified_attributes = params->Get("modified_attributes");

			if (modified_attributes) {
				ObjectLock olock(modified_attributes);
				BOOST_FOREACH(const Dictionary::Pair& kv, modified_attributes) {
					/* update all modified attributes
					 * but do not update the object version yet.
					 * This triggers cluster events otherwise.
					 */
					object->ModifyAttribute(kv.first, kv.second, false);
				}
			}

			/* keep the object version in sync with the sender */
			object->SetVersion(objVersion);
		} else {
			Log(LogNotice, "ApiListener")
			    << "Discarding config update for object '" << object->GetName()
			    << "': Object version " << object->GetVersion()
			    << " is more recent than the received version " << objVersion << ".";
			return Empty;
		}
	}

	return Empty;
}

Value ApiListener::ConfigDeleteObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Log(LogNotice, "ApiListener")
	    << "Received update for object: " << JsonEncode(params);

	/* check permissions */
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return Empty;
	}

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
		    << "Ignoring config update. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiListener")
		    << "Discarding 'config update object' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	Zone::Ptr lzone = Zone::GetLocalZone();
	String objZoneName = params->Get("zone");
	Zone::Ptr objZone = Zone::GetByName(objZoneName);

	/* discard messages if a) not the target zone or b) object zone is not a child of the local zone */
	if (objZone && (lzone->GetName() != objZoneName && !objZone->IsChildOf(lzone))) {
		Log(LogNotice, "ApiListener")
		    << "Discarding 'config update object' message from '"
		    << origin->FromClient->GetIdentity() << "': Object with zone '" << objZoneName << "' not allowed in zone '"
		    << lzone->GetName() << "'.";
		return Empty;
	}

	/* delete the object */
	ConfigType::Ptr dtype = ConfigType::GetByName(params->Get("type"));

	if (!dtype) {
		Log(LogCritical, "ApiListener")
		    << "Config type '" << params->Get("type") << "' does not exist.";
		return Empty;
	}

	ConfigObject::Ptr object = dtype->GetObject(params->Get("name"));

	if (object) {
		if (object->GetPackage() != "_api") {
		    	Log(LogCritical, "ApiListener")
			    << "Could not delete object '" << object->GetName() << "': Not created by the API.";
			return Empty;
		}

		Array::Ptr errors = new Array();
		bool cascade = true; //TODO pass that through the cluster
		if (!ConfigObjectUtility::DeleteObject(object, cascade, errors)) {
			Log(LogCritical, "ApiListener", "Could not delete object:");

			ObjectLock olock(errors);
			BOOST_FOREACH(const String& error, errors) {
				Log(LogCritical, "ApiListener", error);
			}
		}
	} else {
		Log(LogNotice, "ApiListener")
		    << "Could not delete non-existing object '" << params->Get("name") << "'.";
	}

	return Empty;
}

void ApiListener::UpdateConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
    const JsonRpcConnection::Ptr& client)
{
	if (object->GetPackage() != "_api")
		return;

	/* don't sync objects without a zone attribute */
	if (object->GetZoneName().IsEmpty())
		return;

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::UpdateObject");

	Dictionary::Ptr params = new Dictionary();
	params->Set("name", object->GetName());
	params->Set("type", object->GetType()->GetName());
	params->Set("version", object->GetVersion());
	/* required for acceptance criteria on the client */
	params->Set("zone", object->GetZoneName());

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

#ifdef I2_DEBUG
	Log(LogDebug, "ApiListener")
	    << "Sent update for object: " << JsonEncode(params);
#endif /* I2_DEBUG */

	if (client)
		JsonRpc::SendMessage(client->GetStream(), message);
	else
		RelayMessage(origin, object, message, false);
}


void ApiListener::DeleteConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
    const JsonRpcConnection::Ptr& client)
{
	if (object->GetPackage() != "_api")
		return;

	/* don't sync objects without a zone attribute */
	if (object->GetZoneName().IsEmpty())
		return;

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::DeleteObject");

	Dictionary::Ptr params = new Dictionary();
	params->Set("name", object->GetName());
	params->Set("type", object->GetType()->GetName());
	params->Set("version", object->GetVersion());
	/* required for acceptance criteria on the client */
	params->Set("zone", object->GetZoneName());

	message->Set("params", params);

#ifdef I2_DEBUG
	Log(LogDebug, "ApiListener")
	    << "Sent delete object: " << JsonEncode(params);
#endif /* I2_DEBUG */

	if (client)
		JsonRpc::SendMessage(client->GetStream(), message);
	else
		RelayMessage(origin, object, message, false);
}

/* Initial sync on connect for new endpoints */
void ApiListener::SendRuntimeConfigObjects(const JsonRpcConnection::Ptr& aclient)
{
	Endpoint::Ptr endpoint = aclient->GetEndpoint();
	ASSERT(endpoint);

	Zone::Ptr azone = endpoint->GetZone();
	Zone::Ptr lzone = Zone::GetLocalZone();

	Log(LogInformation, "ApiListener")
	    << "Syncing runtime objects to endpoint '" << endpoint->GetName() << "'.";

	BOOST_FOREACH(const ConfigType::Ptr& dt, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, dt->GetObjects()) {
			if (object->GetPackage() != "_api")
				continue;

			String objZone = object->GetZoneName();

			/* don't sync objects without a zone attribute */
			if (objZone.IsEmpty())
				continue;

			/* don't sync objects for non-matching parent-child zones */
			if (!azone->IsChildOf(lzone) && azone != lzone) {
				Log(LogDebug, "ApiListener")
				    << "Skipping sync: object zone '" << objZone
				    << "' defined but client zone '"  << azone->GetName()
				    << "' is not a child of the local zone '" << lzone->GetName() << "'.";
				continue;
			}

			/* send the config object to the connected client */
			UpdateConfigObject(object, MessageOrigin::Ptr(), aclient);
		}
	}
}
