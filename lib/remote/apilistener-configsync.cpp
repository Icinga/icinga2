/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include "config/vmops.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
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

	if (!listener)
		return Empty;

	String objType = params->Get("type");
	String objName = params->Get("name");

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
		    << "Ignoring config update for object '" << objName << "' of type '" << objType << "'. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiListener")
		    << "Discarding 'config update object' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	/* discard messages if the sender is in a child zone */
	if (!Zone::GetLocalZone()->IsChildOf(endpoint->GetZone())) {
		Log(LogWarning, "ApiListener")
		    << "Discarding 'config update object' message from '"
		    << origin->FromClient->GetIdentity() << "'.";
		return Empty;
	}

	/* update the object */
	double objVersion = params->Get("version");

	ConfigType::Ptr dtype = ConfigType::GetByName(objType);

	if (!dtype) {
		Log(LogCritical, "ApiListener")
		    << "Config type '" << objType << "' does not exist.";
		return Empty;
	}

	ConfigObject::Ptr object = dtype->GetObject(objName);

	String config = params->Get("config");

	if (!object && !config.IsEmpty()) {
		/* object does not exist, create it through the API */
		Array::Ptr errors = new Array();

		if (!ConfigObjectUtility::CreateObject(Type::GetByName(objType),
		    objName, config, errors)) {
			Log(LogCritical, "ApiListener")
			    << "Could not create object '" << objName << "':";

		    	ObjectLock olock(errors);
		    	BOOST_FOREACH(const String& error, errors) {
		    		Log(LogCritical, "ApiListener", error);
		    	}

			return Empty;
		}

		object = dtype->GetObject(objName);

		if (!object)
			return Empty;

		/* object was created, update its version */
		object->SetVersion(objVersion, false, origin);
	}

	if (!object)
		return Empty;

	/* update object attributes if version was changed */
	if (objVersion <= object->GetVersion()) {
		Log(LogNotice, "ApiListener")
		    << "Discarding config update for object '" << object->GetName()
		    << "': Object version " << std::fixed << object->GetVersion()
		    << " is more recent than the received version " << std::fixed << objVersion << ".";

		return Empty;
	}

	Log(LogNotice, "ApiListener")
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

	/* check whether original attributes changed and restore them locally */
	Array::Ptr newOriginalAttributes = params->Get("original_attributes");
	Dictionary::Ptr objOriginalAttributes = object->GetOriginalAttributes();

	if (newOriginalAttributes && objOriginalAttributes) {
		std::vector<String> restoreAttrs;

		{
			ObjectLock xlock(objOriginalAttributes);
			BOOST_FOREACH(const Dictionary::Pair& kv, objOriginalAttributes) {
				/* original attribute was removed, restore it */
				if (!newOriginalAttributes->Contains(kv.first))
					restoreAttrs.push_back(kv.first);
			}
		}

		BOOST_FOREACH(const String& key, restoreAttrs) {
			/* do not update the object version yet. */
			object->RestoreAttribute(key, false);
		}
	}

	/* keep the object version in sync with the sender */
	object->SetVersion(objVersion, false, origin);

	return Empty;
}

Value ApiListener::ConfigDeleteObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Log(LogNotice, "ApiListener")
	    << "Received update for object: " << JsonEncode(params);

	/* check permissions */
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

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

	/* discard messages if the sender is in a child zone */
	if (!Zone::GetLocalZone()->IsChildOf(endpoint->GetZone())) {
		Log(LogWarning, "ApiListener")
		    << "Discarding 'config update object' message from '"
		    << origin->FromClient->GetIdentity() << "'.";
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

	if (!object) {
		Log(LogNotice, "ApiListener")
		    << "Could not delete non-existent object '" << params->Get("name") << "'.";
		return Empty;
	}

	if (object->GetPackage() != "_api") {
		Log(LogCritical, "ApiListener")
		    << "Could not delete object '" << object->GetName() << "': Not created by the API.";
		return Empty;
	}

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::DeleteObject(object, true, errors)) {
		Log(LogCritical, "ApiListener", "Could not delete object:");

		ObjectLock olock(errors);
		BOOST_FOREACH(const String& error, errors) {
			Log(LogCritical, "ApiListener", error);
		}
	}

	return Empty;
}

void ApiListener::UpdateConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
    const JsonRpcConnection::Ptr& client)
{
	/* only send objects to zones which have access to the object */
	if (client) {
		Zone::Ptr target_zone = client->GetEndpoint()->GetZone();

		if (target_zone && !target_zone->CanAccessObject(object)) {
			Log(LogDebug, "ApiListener")
			    << "Not sending 'update config' message to unauthorized zone '" << target_zone->GetName() << "'"
			    << " for object: '" << object->GetName() << "'.";

			return;
		}
	}

	if (object->GetPackage() != "_api" && object->GetVersion() == 0)
		return;

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::UpdateObject");

	Dictionary::Ptr params = new Dictionary();
	params->Set("name", object->GetName());
	params->Set("type", object->GetType()->GetName());
	params->Set("version", object->GetVersion());

	if (object->GetPackage() == "_api") {
		String file = ConfigObjectUtility::GetObjectConfigPath(object->GetReflectionType(), object->GetName());

		std::ifstream fp(file.CStr(), std::ifstream::binary);
		if (!fp)
			return;

		String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
		params->Set("config", content);
	}

	Dictionary::Ptr original_attributes = object->GetOriginalAttributes();
	Dictionary::Ptr modified_attributes = new Dictionary();
	Array::Ptr newOriginalAttributes = new Array();

	if (original_attributes) {
		ObjectLock olock(original_attributes);
		BOOST_FOREACH(const Dictionary::Pair& kv, original_attributes) {
			std::vector<String> tokens;
			boost::algorithm::split(tokens, kv.first, boost::is_any_of("."));

			Value value = object;
			BOOST_FOREACH(const String& token, tokens) {
				value = VMOps::GetField(value, token);
			}

			modified_attributes->Set(kv.first, value);

			newOriginalAttributes->Add(kv.first);
		}
	}

	params->Set("modified_attributes", modified_attributes);

	/* only send the original attribute keys */
	params->Set("original_attributes", newOriginalAttributes);

	message->Set("params", params);

#ifdef I2_DEBUG
	Log(LogDebug, "ApiListener")
	    << "Sent update for object '" << object->GetName() << "': " << JsonEncode(params);
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

	/* only send objects to zones which have access to the object */
	if (client) {
		Zone::Ptr target_zone = client->GetEndpoint()->GetZone();

		if (target_zone && !target_zone->CanAccessObject(object)) {
			Log(LogDebug, "ApiListener")
			    << "Not sending 'delete config' message to unauthorized zone '" << target_zone->GetName() << "'"
			    << " for object: '" << object->GetName() << "'.";

			return;
		}
	}

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::DeleteObject");

	Dictionary::Ptr params = new Dictionary();
	params->Set("name", object->GetName());
	params->Set("type", object->GetType()->GetName());
	params->Set("version", object->GetVersion());

	message->Set("params", params);

#ifdef I2_DEBUG
	Log(LogDebug, "ApiListener")
	    << "Sent delete for object '" << object->GetName() << "': " << JsonEncode(params);
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

	Log(LogInformation, "ApiListener")
	    << "Syncing runtime objects to endpoint '" << endpoint->GetName() << "'.";

	BOOST_FOREACH(const ConfigType::Ptr& dt, ConfigType::GetTypes()) {
		BOOST_FOREACH(const ConfigObject::Ptr& object, dt->GetObjects()) {
			/* don't sync objects with an older version time than the endpoint's log position */
			if (object->GetVersion() < endpoint->GetLocalLogPosition())
				continue;

			/* don't sync objects for non-matching parent-child zones */
			if (!azone->CanAccessObject(object))
				continue;

			/* send the config object to the connected client */
			UpdateConfigObject(object, MessageOrigin::Ptr(), aclient);
		}
	}
}
