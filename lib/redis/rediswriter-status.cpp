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

	double startTime = Utility::GetTime();

	//TODO: "Publish" the config dump by adding another event, globally or by object
	ExecuteQuery({ "MULTI" });

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());
		if (!ctype)
			continue;

		String typeName = type->GetName();

		/* replace into aka delete insert is faster than a full diff */
		ExecuteQuery({ "DEL", "icinga:config:" + typeName, "icinga:config:" + typeName + ":checksum", "icinga:status:" + typeName });

		/* fetch all objects and dump them */
		for (const ConfigObject::Ptr& object : ctype->GetObjects()) {
			SendConfigUpdate(object, typeName);
			SendStatusUpdate(object, typeName);
		}

		/* publish config type dump finished */
		ExecuteQuery({ "PUBLISH", "icinga:config:dump", typeName });
	}

	ExecuteQuery({ "EXEC" });

	Log(LogInformation, "RedisWriter")
	    << "Initial config/status dump finished in " << Utility::GetTime() - startTime << " seconds.";
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

	/* Icinga DB part for Icinga Web 2 */
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);

	if (checkable) {
		Dictionary::Ptr attrs = new Dictionary();
		String tableName;
		String objectCheckSum = CalculateCheckSumString(objectName, true); //store binary checksum here

		Host::Ptr host;
		Service::Ptr service;

		tie(host, service) = GetHostService(checkable);

		if (service) {
			tableName = "servicestate";
			attrs->Set("service_checksum", objectCheckSum);
			attrs->Set("host_checksum", CalculateCheckSumString(host->GetName(), true));
		} else {
			tableName = "hoststate";
			attrs->Set("host_checksum", objectCheckSum);
		}

		attrs->Set("last_check", checkable->GetLastCheck());
		attrs->Set("next_check", checkable->GetNextCheck());

		attrs->Set("severity", checkable->GetSeverity());

/*
        'host_checksum'    => null,
        'command'          => null, // JSON, array
        'execution_start'  => null,
        'execution_end'    => null,
        'schedule_start'   => null,
        'schedule_end'     => null,
        'exit_status'      => null,
        'output'           => null,
        'performance_data' => null, // JSON, array


10.0.3.12:6379> keys icinga:hoststate.*
1) "icinga:hoststate.~\xf5a\x91+\x03\x97\x99\xb5(\x16 CYm\xb1\xdf\x85\xa2\xcb"
10.0.3.12:6379> get "icinga:hoststate.~\xf5a\x91+\x03\x97\x99\xb5(\x16 CYm\xb1\xdf\x85\xa2\xcb"
"{\"command\":[\"\\/usr\\/lib\\/nagios\\/plugins\\/check_ping\",\"-H\",\"127.0.0.1\",\"-c\",\"5000,100%\",\"-w\",\"3000,80%\"],\"execution_start\":1492007581.7624,\"execution_end\":1492007585.7654,\"schedule_start\":1492007581.7609,\"schedule_end\":1492007585.7655,\"exit_status\":0,\"output\":\"PING OK - Packet loss = 0%, RTA = 0.08 ms\",\"performance_data\":[\"rta=0.076000ms;3000.000000;5000.000000;0.000000\",\"pl=0%;80;100;0\"]}"

*/

		CheckResult::Ptr cr = checkable->GetLastCheckResult();

		if (cr) {
			attrs->Set("command", JsonEncode(cr->GetCommand()));
			attrs->Set("execution_start", cr->GetExecutionStart());
			attrs->Set("execution_end", cr->GetExecutionEnd());
			attrs->Set("schedule_start", cr->GetScheduleStart());
			attrs->Set("schedule_end", cr->GetScheduleStart());
			attrs->Set("exit_status", cr->GetExitStatus());
			attrs->Set("output", cr->GetOutput());
			attrs->Set("performance_data", JsonEncode(cr->GetPerformanceData()));
		}

		String jsonAttrs = JsonEncode(attrs);
		String key = "icinga:" + tableName + "." + objectCheckSum;
		ExecuteQuery({ "SET", key, jsonAttrs });

		/* expire in check_interval * attempts + timeout + some more seconds */
		double expireTime = checkable->GetCheckInterval() * checkable->GetMaxCheckAttempts() + 60;
		ExecuteQuery({ "EXPIRE", key, String(expireTime) });
	}
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
	} else if (!object->IsActive() && object->GetExtension("ConfigObjectDeleted")) { /* same as in apilistener-configsync.cpp */
		/* Delete object config */
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendConfigDelete, rw.get(), object, type->GetName()));
		}
	}
}
