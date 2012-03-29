#include "i2-jsonrpc.h"

using namespace icinga;

JsonRpcMessage::JsonRpcMessage(void)
{
	m_JSON = NULL;
}

JsonRpcMessage::~JsonRpcMessage(void)
{
	cJSON_Delete(m_JSON);
}

void JsonRpcMessage::SetJSON(cJSON *object)
{
	cJSON_Delete(m_JSON);
	m_JSON = object;
}

cJSON *JsonRpcMessage::GetJSON(void)
{
	return m_JSON;
}

void JsonRpcMessage::InitJson(void)
{
	if (m_JSON == NULL)
		m_JSON = cJSON_CreateObject();
}

void JsonRpcMessage::SetFieldObject(const char *field, cJSON *object)
{
	if (m_JSON == NULL && object == NULL)
		return;

	InitJson();

	cJSON_DeleteItemFromObject(m_JSON, field);

	if (object != NULL)
		cJSON_AddItemToObject(m_JSON, field, object);
}

cJSON *JsonRpcMessage::GetFieldObject(const char *field)
{
	if (m_JSON == NULL)
		return NULL;

	return cJSON_GetObjectItem(m_JSON, field);
}

void JsonRpcMessage::SetFieldString(const char *field, const string& value)
{
	cJSON *object = cJSON_CreateString(value.c_str());
	SetFieldObject(field, object);
}

string JsonRpcMessage::GetFieldString(const char *field)
{
	cJSON *object = GetFieldObject(field);

	if (object == NULL || object->type != cJSON_String)
		return string();

	return string(object->valuestring);
}

void JsonRpcMessage::SetVersion(const string& version)
{
	SetFieldString("version", version);
}

string JsonRpcMessage::GetVersion(void)
{
	return GetFieldString("jsonrpc");
}

void JsonRpcMessage::SetID(const string& id)
{
	SetFieldString("id", id);
}

string JsonRpcMessage::GetID(void)
{
	return GetFieldString("id");
}

void JsonRpcMessage::SetMethod(const string& method)
{
	SetFieldString("method", method);
}

string JsonRpcMessage::GetMethod(void)
{
	return GetFieldString("method");
}

void JsonRpcMessage::ClearParams(void)
{
	SetFieldObject("params", NULL);
}

cJSON *JsonRpcMessage::GetParams(void)
{
	cJSON *object = GetFieldObject("params");

	if (object == NULL) {
		object = cJSON_CreateObject();
		cJSON_AddItemToObject(m_JSON, "params", object);
	}

	return object;
}

void JsonRpcMessage::ClearResult(void)
{
	SetFieldObject("result", NULL);
}

cJSON *JsonRpcMessage::GetResult(void)
{
	cJSON *object = GetFieldObject("result");

	if (object == NULL) {
		object = cJSON_CreateObject();
		cJSON_AddItemToObject(m_JSON, "result", object);
	}

	return object;
}

void JsonRpcMessage::SetError(const string& error)
{
	SetFieldString("error", error);
}

string JsonRpcMessage::GetError(void)
{
	return GetFieldString("error");
}
