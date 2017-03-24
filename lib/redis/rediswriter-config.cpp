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

#include "redis/rediswriter.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/tlsutility.hpp"
#include "base/initialize.hpp"

using namespace icinga;

/*
- icinga:config:<type> as hash
key: sha1 checksum(name)
value: JsonEncode(Serialize(object, FAConfig)) + config_checksum

Diff between calculated config_checksum and Redis json config_checksum
Alternative: Replace into.


- icinga:status:<type> as hash
key: sha1 checksum(name)
value: JsonEncode(Serialize(object, FAState))
*/

INITIALIZE_ONCE(&RedisWriter::ConfigStaticInitialize);

void RedisWriter::ConfigStaticInitialize(void)
{
	/* triggered in ProcessCheckResult(), requires UpdateNextCheck() to be called before */
	ConfigObject::OnStateChanged.connect(boost::bind(&RedisWriter::StateChangedHandler, _1));
	CustomVarObject::OnVarsChanged.connect(boost::bind(&RedisWriter::VarsChangedHandler, _1));

	/* triggered on create, update and delete objects */
	ConfigObject::OnActiveChanged.connect(boost::bind(&RedisWriter::VersionChangedHandler, _1));
	ConfigObject::OnVersionChanged.connect(boost::bind(&RedisWriter::VersionChangedHandler, _1));
}

void RedisWriter::UpdateAllConfigObjects(void)
{
	AssertOnWorkQueue();

	//TODO: "Publish" the config dump by adding another event, globally or by object
	ExecuteQuery({ "MULTI" });

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		if (!ConfigObject::TypeInstance->IsAssignableFrom(type))
			continue;

		String typeName = type->GetName();

		/* replace into aka delete insert is faster than a full diff */
		ExecuteQuery({ "DEL", "icinga:config:" + typeName, "icinga:config:" + typeName + ":checksum", "icinga:status:" + typeName });

		/* fetch all objects and dump them */
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());
		VERIFY(ctype);

		for (const ConfigObject::Ptr& object : ctype->GetObjects()) {
			SendConfigUpdate(object, typeName);
			SendStatusUpdate(object, typeName);
		}

		/* publish config type dump finished */
		ExecuteQuery({ "PUBLISH", "icinga:config:dump", typeName });
	}

	ExecuteQuery({ "EXEC" });
}

void RedisWriter::SendConfigUpdate(const ConfigObject::Ptr& object, const String& typeName, bool runtimeUpdate)
{
	AssertOnWorkQueue();

	/* during startup we might send duplicated object config, ignore them without any connection */
	if (!m_Context)
		return;

	/* TODO: This isn't essentially correct as we don't keep track of config objects ourselves. This would avoid duplicated config updates at startup.
	if (!runtimeUpdate && m_ConfigDumpInProgress)
		return;
	*/

	/* Serialize config object attributes */
	Dictionary::Ptr objectAttrs = SerializeObjectAttrs(object, FAConfig);

	String jsonBody = JsonEncode(objectAttrs);

	String objectName = object->GetName();

	ExecuteQuery({ "HSET", "icinga:config:" + typeName, objectName, jsonBody });

	/* check sums */
	/* hset icinga:config:Host:checksums localhost { "name_checksum": "...", "properties_checksum": "...", "groups_checksum": "...", "vars_checksum": null } */
	Dictionary::Ptr checkSum = new Dictionary();

	checkSum->Set("name_checksum", CalculateCheckSumString(object->GetName()));

	// TODO: move this elsewhere
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (checkable) {
		Host::Ptr host;
		Service::Ptr service;

		tie(host, service) = GetHostService(checkable);

		if (service)
			checkSum->Set("groups_checksum", CalculateCheckSumGroups(service->GetGroups()));
		else
			checkSum->Set("groups_checksum", CalculateCheckSumGroups(host->GetGroups()));
	}

	checkSum->Set("properties_checksum", CalculateCheckSumProperties(object));
	checkSum->Set("vars_checksum", CalculateCheckSumVars(object));

	String checkSumBody = JsonEncode(checkSum);

	ExecuteQuery({ "HSET", "icinga:config:" + typeName + ":checksum", objectName, checkSumBody });

	/* publish runtime updated objects immediately */
	if (!runtimeUpdate)
		return;

	/*
	PUBLISH "icinga:config:dump" "Host"
	PUBLISH "icinga:config:update" "Host:__name!checksumBody"
	PUBLISH "icinga:config:delete" "Host:__name"
	*/

	ExecuteQuery({ "PUBLISH", "icinga:config:update", typeName + ":" + objectName + "!" + checkSumBody });
}

void RedisWriter::SendConfigDelete(const ConfigObject::Ptr& object, const String& typeName)
{
	AssertOnWorkQueue();

	/* during startup we might send duplicated object config, ignore them without any connection */
	if (!m_Context)
		return;

	String objectName = object->GetName();

	ExecuteQuery({ "HDEL", "icinga:config:" + typeName, objectName });
	ExecuteQuery({ "HDEL", "icinga:config:" + typeName + ":checksum", objectName });
	ExecuteQuery({ "HDEL", "icinga:status:" + typeName, objectName });

	/*
	PUBLISH "icinga:config:dump" "Host"
	PUBLISH "icinga:config:update" "Host:__name!checksumBody"
	PUBLISH "icinga:config:delete" "Host:__name"
	*/

	ExecuteQuery({ "PUBLISH", "icinga:config:delete", typeName + ":" + objectName });
}

void RedisWriter::SendStatusUpdate(const ConfigObject::Ptr& object, const String& typeName)
{
	AssertOnWorkQueue();

	/* during startup we might receive check results, ignore them without any connection */
	if (!m_Context)
		return;

	/* Serialize config object attributes */
	Dictionary::Ptr objectAttrs = SerializeObjectAttrs(object, FAState);

	String jsonBody = JsonEncode(objectAttrs);

	String objectName = object->GetName();

	ExecuteQuery({ "HSET", "icinga:status:" + typeName, objectName, jsonBody });
}

void RedisWriter::StateChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
		rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendStatusUpdate, rw, object, type->GetName()));
	}
}

void RedisWriter::VarsChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
		rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendConfigUpdate, rw, object, type->GetName(), true));
	}
}

void RedisWriter::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	if (object->IsActive()) {
		/* Create or update the object config */
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendConfigUpdate, rw.get(), object, type->GetName(), true));
		}
	} else {
		/* Delete object config */
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendConfigDelete, rw.get(), object, type->GetName()));
		}
	}
}
