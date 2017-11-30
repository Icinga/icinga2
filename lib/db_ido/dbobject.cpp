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

#include "db_ido/dbobject.hpp"
#include "db_ido/dbtype.hpp"
#include "db_ido/dbvalue.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/service.hpp"
#include "icinga/compatutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "remote/endpoint.hpp"
#include "base/configobject.hpp"
#include "base/configtype.hpp"
#include "base/json.hpp"
#include "base/serializer.hpp"
#include "base/json.hpp"
#include "base/convert.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/initialize.hpp"
#include "base/logger.hpp"

using namespace icinga;

boost::signals2::signal<void (const DbQuery&)> DbObject::OnQuery;
boost::signals2::signal<void (const std::vector<DbQuery>&)> DbObject::OnMultipleQueries;

INITIALIZE_ONCE(&DbObject::StaticInitialize);

DbObject::DbObject(const intrusive_ptr<DbType>& type, const String& name1, const String& name2)
	: m_Name1(name1), m_Name2(name2), m_Type(type), m_LastConfigUpdate(0), m_LastStatusUpdate(0)
{ }

void DbObject::StaticInitialize(void)
{
	/* triggered in ProcessCheckResult(), requires UpdateNextCheck() to be called before */
	ConfigObject::OnStateChanged.connect(std::bind(&DbObject::StateChangedHandler, _1));
	CustomVarObject::OnVarsChanged.connect(std::bind(&DbObject::VarsChangedHandler, _1));

	/* triggered on create, update and delete objects */
	ConfigObject::OnVersionChanged.connect(std::bind(&DbObject::VersionChangedHandler, _1));
}

void DbObject::SetObject(const ConfigObject::Ptr& object)
{
	m_Object = object;
}

ConfigObject::Ptr DbObject::GetObject(void) const
{
	return m_Object;
}

String DbObject::GetName1(void) const
{
	return m_Name1;
}

String DbObject::GetName2(void) const
{
	return m_Name2;
}

DbType::Ptr DbObject::GetType(void) const
{
	return m_Type;
}

String DbObject::CalculateConfigHash(const Dictionary::Ptr& configFields) const
{
	Dictionary::Ptr configFieldsDup = configFields->ShallowClone();

	{
		ObjectLock olock(configFieldsDup);

		for (const Dictionary::Pair& kv : configFieldsDup) {
			if (kv.second.IsObjectType<ConfigObject>()) {
				ConfigObject::Ptr obj = kv.second;
				configFieldsDup->Set(kv.first, obj->GetName());
			}
		}
	}

	Array::Ptr data = new Array();
	data->Add(configFieldsDup);

	CustomVarObject::Ptr custom_var_object = dynamic_pointer_cast<CustomVarObject>(GetObject());

	if (custom_var_object)
		data->Add(custom_var_object->GetVars());

	return HashValue(data);
}

String DbObject::HashValue(const Value& value)
{
	Value temp;

	Type::Ptr type = value.GetReflectionType();

	if (ConfigObject::TypeInstance->IsAssignableFrom(type))
		temp = Serialize(value, FAConfig);
	else
		temp = value;

	return SHA256(JsonEncode(temp));
}

void DbObject::SendConfigUpdateHeavy(const Dictionary::Ptr& configFields)
{
	/* update custom var config and status */
	SendVarsConfigUpdateHeavy();
	SendVarsStatusUpdate();

	/* config attributes */
	if (!configFields)
		return;

	ASSERT(configFields->Contains("config_hash"));

	ConfigObject::Ptr object = GetObject();

	DbQuery query;
	query.Table = GetType()->GetTable() + "s";
	query.Type = DbQueryInsert | DbQueryUpdate;
	query.Category = DbCatConfig;
	query.Fields = configFields;
	query.Fields->Set(GetType()->GetIDColumn(), object);
	query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */
	query.Fields->Set("config_type", 1);
	query.WhereCriteria = new Dictionary();
	query.WhereCriteria->Set(GetType()->GetIDColumn(), object);
	query.Object = this;
	query.ConfigUpdate = true;
	OnQuery(query);

	m_LastConfigUpdate = Utility::GetTime();

	OnConfigUpdateHeavy();
}

void DbObject::SendConfigUpdateLight(void)
{
	OnConfigUpdateLight();
}

void DbObject::SendStatusUpdate(void)
{
	/* status attributes */
	Dictionary::Ptr fields = GetStatusFields();

	if (!fields)
		return;

	DbQuery query;
	query.Table = GetType()->GetTable() + "status";
	query.Type = DbQueryInsert | DbQueryUpdate;
	query.Category = DbCatState;
	query.Fields = fields;
	query.Fields->Set(GetType()->GetIDColumn(), GetObject());

	/* do not override endpoint_object_id for endpoints & zones */
	if (query.Table != "endpointstatus" && query.Table != "zonestatus") {
		String node = IcingaApplication::GetInstance()->GetNodeName();

		Endpoint::Ptr endpoint = Endpoint::GetByName(node);
		if (endpoint)
			query.Fields->Set("endpoint_object_id", endpoint);
	}

	query.Fields->Set("instance_id", 0); /* DbConnection class fills in real ID */

	query.Fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
	query.WhereCriteria = new Dictionary();
	query.WhereCriteria->Set(GetType()->GetIDColumn(), GetObject());
	query.Object = this;
	query.StatusUpdate = true;
	OnQuery(query);

	m_LastStatusUpdate = Utility::GetTime();

	OnStatusUpdate();
}

void DbObject::SendVarsConfigUpdateHeavy(void)
{
	ConfigObject::Ptr obj = GetObject();

	CustomVarObject::Ptr custom_var_object = dynamic_pointer_cast<CustomVarObject>(obj);

	if (!custom_var_object)
		return;

	std::vector<DbQuery> queries;

	DbQuery query1;
	query1.Table = "customvariables";
	query1.Type = DbQueryDelete;
	query1.Category = DbCatConfig;
	query1.WhereCriteria = new Dictionary();
	query1.WhereCriteria->Set("object_id", obj);
	queries.emplace_back(std::move(query1));

	DbQuery query2;
	query2.Table = "customvariablestatus";
	query2.Type = DbQueryDelete;
	query2.Category = DbCatConfig;
	query2.WhereCriteria = new Dictionary();
	query2.WhereCriteria->Set("object_id", obj);
	queries.emplace_back(std::move(query2));

	Dictionary::Ptr vars = CompatUtility::GetCustomAttributeConfig(custom_var_object);

	if (vars) {
		ObjectLock olock (vars);

		for (const Dictionary::Pair& kv : vars) {
			if (kv.first.IsEmpty())
				continue;

			String value;
			int is_json = 0;

			if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>()) {
				value = JsonEncode(kv.second);
				is_json = 1;
			} else
				value = kv.second;

			Dictionary::Ptr fields = new Dictionary();
			fields->Set("varname", kv.first);
			fields->Set("varvalue", value);
			fields->Set("is_json", is_json);
			fields->Set("config_type", 1);
			fields->Set("object_id", obj);
			fields->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query3;
			query3.Table = "customvariables";
			query3.Type = DbQueryInsert;
			query3.Category = DbCatConfig;
			query3.Fields = fields;
			queries.emplace_back(std::move(query3));
		}
	}

	OnMultipleQueries(queries);
}

void DbObject::SendVarsStatusUpdate(void)
{
	ConfigObject::Ptr obj = GetObject();

	CustomVarObject::Ptr custom_var_object = dynamic_pointer_cast<CustomVarObject>(obj);

	if (!custom_var_object)
		return;

	Dictionary::Ptr vars = CompatUtility::GetCustomAttributeConfig(custom_var_object);

	if (vars) {
		std::vector<DbQuery> queries;
		ObjectLock olock (vars);

		for (const Dictionary::Pair& kv : vars) {
			if (kv.first.IsEmpty())
				continue;

			String value;
			int is_json = 0;

			if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>()) {
				value = JsonEncode(kv.second);
				is_json = 1;
			} else
				value = kv.second;

			Dictionary::Ptr fields = new Dictionary();
			fields->Set("varname", kv.first);
			fields->Set("varvalue", value);
			fields->Set("is_json", is_json);
			fields->Set("status_update_time", DbValue::FromTimestamp(Utility::GetTime()));
			fields->Set("object_id", obj);
			fields->Set("instance_id", 0); /* DbConnection class fills in real ID */

			DbQuery query;
			query.Table = "customvariablestatus";
			query.Type = DbQueryInsert | DbQueryUpdate;
			query.Category = DbCatState;
			query.Fields = fields;

			query.WhereCriteria = new Dictionary();
			query.WhereCriteria->Set("object_id", obj);
			query.WhereCriteria->Set("varname", kv.first);
			queries.emplace_back(std::move(query));
		}

		OnMultipleQueries(queries);
	}
}

double DbObject::GetLastConfigUpdate(void) const
{
	return m_LastConfigUpdate;
}

double DbObject::GetLastStatusUpdate(void) const
{
	return m_LastStatusUpdate;
}

void DbObject::OnConfigUpdateHeavy(void)
{
	/* Default handler does nothing. */
}

void DbObject::OnConfigUpdateLight(void)
{
	/* Default handler does nothing. */
}

void DbObject::OnStatusUpdate(void)
{
	/* Default handler does nothing. */
}

DbObject::Ptr DbObject::GetOrCreateByObject(const ConfigObject::Ptr& object)
{
	boost::mutex::scoped_lock lock(GetStaticMutex());

	DbObject::Ptr dbobj = object->GetExtension("DbObject");

	if (dbobj)
		return dbobj;

	DbType::Ptr dbtype = DbType::GetByName(object->GetReflectionType()->GetName());

	if (!dbtype)
		return DbObject::Ptr();

	Service::Ptr service;
	String name1, name2;

	service = dynamic_pointer_cast<Service>(object);

	if (service) {
		Host::Ptr host = service->GetHost();

		name1 = service->GetHost()->GetName();
		name2 = service->GetShortName();
	} else {
		if (object->GetReflectionType() == CheckCommand::TypeInstance ||
		    object->GetReflectionType() == EventCommand::TypeInstance ||
		    object->GetReflectionType() == NotificationCommand::TypeInstance) {
			Command::Ptr command = dynamic_pointer_cast<Command>(object);
			name1 = CompatUtility::GetCommandName(command);
		}
		else
			name1 = object->GetName();
	}

	dbobj = dbtype->GetOrCreateObjectByName(name1, name2);

	dbobj->SetObject(object);
	object->SetExtension("DbObject", dbobj);

	return dbobj;
}

void DbObject::StateChangedHandler(const ConfigObject::Ptr& object)
{
	DbObject::Ptr dbobj = GetOrCreateByObject(object);

	if (!dbobj)
		return;

	dbobj->SendStatusUpdate();
}

void DbObject::VarsChangedHandler(const CustomVarObject::Ptr& object)
{
	DbObject::Ptr dbobj = GetOrCreateByObject(object);

	if (!dbobj)
		return;

	dbobj->SendVarsStatusUpdate();
}

void DbObject::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	DbObject::Ptr dbobj = DbObject::GetOrCreateByObject(object);

	if (dbobj) {
		Dictionary::Ptr configFields = dbobj->GetConfigFields();
		String configHash = dbobj->CalculateConfigHash(configFields);
		configFields->Set("config_hash", configHash);

		dbobj->SendConfigUpdateHeavy(configFields);
		dbobj->SendStatusUpdate();
	}
}

boost::mutex& DbObject::GetStaticMutex(void)
{
	static boost::mutex mutex;
	return mutex;
}
