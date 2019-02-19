/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "remote/configobjectutility.hpp"
#include "remote/jsonrpc.hpp"
#include "base/configtype.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "config/vmops.hpp"
#include <fstream>

using namespace icinga;

REGISTER_APIFUNCTION(UpdateObject, config, &ApiListener::ConfigUpdateObjectAPIHandler);
REGISTER_APIFUNCTION(DeleteObject, config, &ApiListener::ConfigDeleteObjectAPIHandler);

INITIALIZE_ONCE([]() {
	ConfigObject::OnActiveChanged.connect(&ApiListener::ConfigUpdateObjectHandler);
	ConfigObject::OnVersionChanged.connect(&ApiListener::ConfigUpdateObjectHandler);
});

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

	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	/* discard messages if the client is not configured on this node */
	if (!endpoint) {
		Log(LogNotice, "ApiListener")
			<< "Discarding 'config update object' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	/* discard messages if the sender is in a child zone */
	if (!Zone::GetLocalZone()->IsChildOf(endpoint->GetZone())) {
		Log(LogNotice, "ApiListener")
			<< "Discarding 'config update object' message from '"
			<< origin->FromClient->GetIdentity() << "' for object '"
			<< objName << "' of type '" << objType << "'. Sender is in a child zone.";
		return Empty;
	}

	/* ignore messages if the endpoint does not accept config */
	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
			<< "Ignoring config update from '" << origin->FromClient->GetIdentity() << "' for object '" << objName << "' of type '" << objType << "'. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	/* update the object */
	double objVersion = params->Get("version");

	Type::Ptr ptype = Type::GetByName(objType);
	auto *ctype = dynamic_cast<ConfigType *>(ptype.get());

	if (!ctype) {
		Log(LogCritical, "ApiListener")
			<< "Config type '" << objType << "' does not exist.";
		return Empty;
	}

	ConfigObject::Ptr object = ctype->GetObject(objName);

	String config = params->Get("config");

	bool newObject = false;

	if (!object && !config.IsEmpty()) {
		newObject = true;

		/* object does not exist, create it through the API */
		Array::Ptr errors = new Array();

		if (!ConfigObjectUtility::CreateObject(ptype, objName, config, errors, nullptr)) {
			Log(LogCritical, "ApiListener")
				<< "Could not create object '" << objName << "':";

			ObjectLock olock(errors);
			for (const String& error : errors) {
				Log(LogCritical, "ApiListener", error);
			}

			return Empty;
		}

		object = ctype->GetObject(objName);

		if (!object)
			return Empty;

		/* object was created, update its version */
		object->SetVersion(objVersion, false, origin);
	}

	if (!object)
		return Empty;

	/* update object attributes if version was changed or if this is a new object */
	if (newObject || objVersion <= object->GetVersion()) {
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
		for (const Dictionary::Pair& kv : modified_attributes) {
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
			for (const Dictionary::Pair& kv : objOriginalAttributes) {
				/* original attribute was removed, restore it */
				if (!newOriginalAttributes->Contains(kv.first))
					restoreAttrs.push_back(kv.first);
			}
		}

		for (const String& key : restoreAttrs) {
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
		<< "Received delete for object: " << JsonEncode(params);

	/* check permissions */
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return Empty;

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
			<< "Ignoring config delete. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	Endpoint::Ptr endpoint = origin->FromClient->GetEndpoint();

	if (!endpoint) {
		Log(LogNotice, "ApiListener")
			<< "Discarding 'config delete object' message from '" << origin->FromClient->GetIdentity() << "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	/* discard messages if the sender is in a child zone */
	if (!Zone::GetLocalZone()->IsChildOf(endpoint->GetZone())) {
		Log(LogNotice, "ApiListener")
			<< "Discarding 'config delete object' message from '"
			<< origin->FromClient->GetIdentity() << "'.";
		return Empty;
	}

	/* delete the object */
	Type::Ptr ptype = Type::GetByName(params->Get("type"));
	auto *ctype = dynamic_cast<ConfigType *>(ptype.get());

	if (!ctype) {
		Log(LogCritical, "ApiListener")
			<< "Config type '" << params->Get("type") << "' does not exist.";
		return Empty;
	}

	ConfigObject::Ptr object = ctype->GetObject(params->Get("name"));

	if (!object) {
		Log(LogNotice, "ApiListener")
			<< "Could not delete non-existent object '" << params->Get("name") << "' with type '" << params->Get("type") << "'.";
		return Empty;
	}

	if (object->GetPackage() != "_api") {
		Log(LogCritical, "ApiListener")
			<< "Could not delete object '" << object->GetName() << "': Not created by the API.";
		return Empty;
	}

	Array::Ptr errors = new Array();

	if (!ConfigObjectUtility::DeleteObject(object, true, errors, nullptr)) {
		Log(LogCritical, "ApiListener", "Could not delete object:");

		ObjectLock olock(errors);
		for (const String& error : errors) {
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

	Dictionary::Ptr params = new Dictionary();

	Dictionary::Ptr message = new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::UpdateObject" },
		{ "params", params }
	});

	params->Set("name", object->GetName());
	params->Set("type", object->GetReflectionType()->GetName());
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
	ArrayData newOriginalAttributes;

	if (original_attributes) {
		ObjectLock olock(original_attributes);
		for (const Dictionary::Pair& kv : original_attributes) {
			std::vector<String> tokens = kv.first.Split(".");

			Value value = object;
			for (const String& token : tokens) {
				value = VMOps::GetField(value, token);
			}

			modified_attributes->Set(kv.first, value);

			newOriginalAttributes.push_back(kv.first);
		}
	}

	params->Set("modified_attributes", modified_attributes);

	/* only send the original attribute keys */
	params->Set("original_attributes", new Array(std::move(newOriginalAttributes)));

#ifdef I2_DEBUG
	Log(LogDebug, "ApiListener")
		<< "Sent update for object '" << object->GetName() << "': " << JsonEncode(params);
#endif /* I2_DEBUG */

	if (client)
		client->SendMessage(message);
	else {
		Zone::Ptr target = static_pointer_cast<Zone>(object->GetZone());

		if (!target)
			target = Zone::GetLocalZone();

		RelayMessage(origin, target, message, false);
	}
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

	Dictionary::Ptr params = new Dictionary();

	Dictionary::Ptr message = new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::DeleteObject" },
		{ "params", params }
	});

	params->Set("name", object->GetName());
	params->Set("type", object->GetReflectionType()->GetName());
	params->Set("version", object->GetVersion());


#ifdef I2_DEBUG
	Log(LogDebug, "ApiListener")
		<< "Sent delete for object '" << object->GetName() << "': " << JsonEncode(params);
#endif /* I2_DEBUG */

	if (client)
		client->SendMessage(message);
	else {
		Zone::Ptr target = static_pointer_cast<Zone>(object->GetZone());

		if (!target)
			target = Zone::GetLocalZone();

		RelayMessage(origin, target, message, false);
	}
}

/* Initial sync on connect for new endpoints */
void ApiListener::SendRuntimeConfigObjects(const JsonRpcConnection::Ptr& aclient)
{
	Endpoint::Ptr endpoint = aclient->GetEndpoint();
	ASSERT(endpoint);

	Zone::Ptr azone = endpoint->GetZone();

	Log(LogInformation, "ApiListener")
		<< "Syncing runtime objects to endpoint '" << endpoint->GetName() << "'.";

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			/* don't sync objects for non-matching parent-child zones */
			if (!azone->CanAccessObject(object))
				continue;

			/* send the config object to the connected client */
			UpdateConfigObject(object, nullptr, aclient);
		}
	}

	Log(LogInformation, "ApiListener")
		<< "Finished syncing runtime objects to endpoint '" << endpoint->GetName() << "'.";
}
