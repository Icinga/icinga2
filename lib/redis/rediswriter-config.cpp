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
	ConfigObject::OnVersionChanged.connect(boost::bind(&RedisWriter::VersionChangedHandler, _1));
}

void RedisWriter::UpdateAllConfigObjects(void)
{
	AssertOnWorkQueue();

	redisReply *reply1 = reinterpret_cast<redisReply *>(redisCommand(m_Context, "MULTI"));

	if (!reply1) {
		redisFree(m_Context);
		m_Context = NULL;
		return;
	}

	if (reply1->type == REDIS_REPLY_ERROR) {
		Log(LogInformation, "RedisWriter")
		    << "MULTI: " << reply1->str;
	}

	if (reply1->type == REDIS_REPLY_ERROR) {
		freeReplyObject(reply1);
		return;
	}

	freeReplyObject(reply1);

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		if (!ConfigObject::TypeInstance->IsAssignableFrom(type))
			continue;

		String typeName = type->GetName();

		/* replace into aka delete insert is faster than a full diff */
		redisReply *reply2 = reinterpret_cast<redisReply *>(redisCommand(m_Context, "DEL icinga:config:%s icinga:status:%s", typeName.CStr(), typeName.CStr()));

		if (!reply2) {
			redisFree(m_Context);
			m_Context = NULL;
			return;
		}

		if (reply2->type == REDIS_REPLY_ERROR) {
			Log(LogInformation, "RedisWriter")
			    << "DEL icinga:config:" << typeName << " icinga:status:" << typeName << ": " << reply2->str;
		}

		if (reply2->type == REDIS_REPLY_ERROR) {
			freeReplyObject(reply2);
			return;
		}

		freeReplyObject(reply2);

		/* fetch all objects and dump them */
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());
		VERIFY(ctype);

		for (const ConfigObject::Ptr& object : ctype->GetObjects()) {
			SendConfigUpdate(object, typeName);
			SendStatusUpdate(object, typeName);
		}
	}

	redisReply *reply3 = reinterpret_cast<redisReply *>(redisCommand(m_Context, "EXEC"));

	if (!reply3) {
		redisFree(m_Context);
		m_Context = NULL;
		return;
	}

	if (reply3->type == REDIS_REPLY_ERROR) {
		Log(LogInformation, "RedisWriter")
		    << "EXEC: " << reply3->str;
	}

	if (reply3->type == REDIS_REPLY_ERROR) {
		freeReplyObject(reply3);
		return;
	}

	freeReplyObject(reply3);
}

void RedisWriter::SendConfigUpdate(const ConfigObject::Ptr& object, const String& typeName)
{
	AssertOnWorkQueue();

	/* Serialize config object attributes */
	Dictionary::Ptr objectAttrs = SerializeObjectAttrs(object, FAConfig);

	String jsonBody = JsonEncode(objectAttrs);

	//TODO: checksum
	String objectName = object->GetName();

	redisReply *reply1 = reinterpret_cast<redisReply *>(redisCommand(m_Context, "HSET icinga:config:%s %s %s", typeName.CStr(), objectName.CStr(), jsonBody.CStr()));

	if (!reply1) {
		redisFree(m_Context);
		m_Context = NULL;
		return;
	}

	if (reply1->type == REDIS_REPLY_ERROR) {
		Log(LogInformation, "RedisWriter")
		    << "HSET icinga:config:" << typeName << " " << objectName << " " << jsonBody << ": " << reply1->str;
	}

	if (reply1->type == REDIS_REPLY_ERROR) {
		freeReplyObject(reply1);
		return;
	}

	freeReplyObject(reply1);


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

	String checkSumBody = JsonEncode(checkSum);

	redisReply *reply2 = reinterpret_cast<redisReply *>(redisCommand(m_Context, "HSET icinga:config:%s:checksum %s %s", typeName.CStr(), objectName.CStr(), checkSumBody.CStr()));

	if (!reply2) {
		redisFree(m_Context);
		m_Context = NULL;
		return;
	}

	if (reply2->type == REDIS_REPLY_ERROR) {
		Log(LogInformation, "RedisWriter")
		    << "HSET icinga:config:" << typeName << " " << objectName << " " << jsonBody << ": " << reply2->str;
	}

	if (reply2->type == REDIS_REPLY_ERROR) {
		freeReplyObject(reply2);
		return;
	}

	freeReplyObject(reply2);
}

void RedisWriter::SendStatusUpdate(const ConfigObject::Ptr& object, const String& typeName)
{
	AssertOnWorkQueue();

	/* Serialize config object attributes */
	Dictionary::Ptr objectAttrs = SerializeObjectAttrs(object, FAState);

	String jsonBody = JsonEncode(objectAttrs);

	//TODO: checksum
	String objectName = object->GetName();

	redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "HSET icinga:status:%s %s %s", typeName.CStr(), objectName.CStr(), jsonBody.CStr()));

	if (!reply) {
		redisFree(m_Context);
		m_Context = NULL;
		return;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		Log(LogInformation, "RedisWriter")
		    << "HSET icinga:status:" << typeName << " " << objectName << " " << jsonBody << ": " << reply->str;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		freeReplyObject(reply);
		return;
	}

	freeReplyObject(reply);
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
		rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendConfigUpdate, rw, object, type->GetName()));
	}
}

void RedisWriter::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
		rw->m_WorkQueue.Enqueue(boost::bind(&RedisWriter::SendConfigUpdate, rw.get(), object, type->GetName()));
	}
}
