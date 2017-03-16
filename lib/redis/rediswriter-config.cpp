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
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"

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

//TODO: OnActiveChanged handling.
void RedisWriter::UpdateAllConfigObjects(void)
{
	//TODO: Just use config types
	for (const Type::Ptr& type : Type::GetAllTypes()) {
		if (!ConfigObject::TypeInstance->IsAssignableFrom(type))
			continue;

		String typeName = type->GetName();

		/* replace into aka delete insert is faster than a full diff */
		Log(LogInformation, "RedisWriter")
		    << "Flushing icinga:config:" << typeName << " before config dump.";

		redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "DEL icinga:config:%s", typeName.CStr()));

		if (!reply) {
			redisFree(m_Context);
			m_Context = NULL;
			return;
		}

		if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
			Log(LogInformation, "RedisWriter")
			    << "DEL icinga:config:" << typeName << ": " << reply->str;
		}

		if (reply->type == REDIS_REPLY_ERROR) {
			freeReplyObject(reply);
			return;
		}

		freeReplyObject(reply);

		/* fetch all objects and dump them */
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());

		if (ctype) {
			for (const ConfigObject::Ptr& object : ctype->GetObjects()) {
				DumpConfigObject(object, typeName);
			}
		}
	}
}

void RedisWriter::DumpConfigObject(const ConfigObject::Ptr& object, const String& typeName)
{
	/* Serialize config object attributes */
	Dictionary::Ptr objectAttrs = SerializeObjectAttrs(object, FAConfig);

	String jsonBody = JsonEncode(objectAttrs);

	//TODO: checksum
	String objectName = object->GetName();

	redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "HSET icinga:config:%s %s %s", typeName.CStr(), objectName.CStr(), jsonBody.CStr()));

	if (!reply) {
		redisFree(m_Context);
		m_Context = NULL;
		return;
	}

	if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
		Log(LogInformation, "RedisWriter")
		    << "HSET icinga:config:" << typeName << " " << objectName << " " << jsonBody << ": " << reply->str;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		freeReplyObject(reply);
		return;
	}

	freeReplyObject(reply);
}

Dictionary::Ptr RedisWriter::SerializeObjectAttrs(const Object::Ptr& object, int fieldType)
{
	Type::Ptr type = object->GetReflectionType();

	std::vector<int> fids;

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		fids.push_back(fid);
	}

	Dictionary::Ptr resultAttrs = new Dictionary();

	for (int& fid : fids)
	{
		Field field = type->GetFieldInfo(fid);

		Value val = object->GetField(fid);

		/* hide attributes which shouldn't be user-visible */
		if (field.Attributes & FANoUserView)
			continue;

		/* hide internal navigation fields */
		if (field.Attributes & FANavigation && !(field.Attributes & (FAConfig | FAState)))
			continue;

		Value sval = Serialize(val, fieldType);
		resultAttrs->Set(field.Name, sval);
	}

	return resultAttrs;
}

