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

void JsonRpcMessage::SetFieldString(const char *field, const string& value)
{
	if (m_JSON == NULL)
		m_JSON = cJSON_CreateObject();

	cJSON *object = cJSON_CreateString(value.c_str());
	cJSON_DeleteItemFromObject(m_JSON, field);
	cJSON_AddItemToObject(m_JSON, field, object);
}

string JsonRpcMessage::GetFieldString(const char *field)
{
	if (m_JSON == NULL)
		m_JSON = cJSON_CreateObject();

	cJSON *idObject = cJSON_GetObjectItem(m_JSON, field);

	if (idObject == NULL || idObject->type != cJSON_String)
		return string();

	return string(idObject->valuestring);
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

void JsonRpcMessage::SetParams(const string& params)
{
	SetFieldString("params", params);
}

string JsonRpcMessage::GetParams(void)
{
	return GetFieldString("params");
}

void JsonRpcMessage::SetResult(const string& result)
{
	SetFieldString("result", result);
}

string JsonRpcMessage::GetResult(void)
{
	return GetFieldString("result");
}

void JsonRpcMessage::SetError(const string& error)
{
	SetFieldString("error", error);
}

string JsonRpcMessage::GetError(void)
{
	return GetFieldString("error");
}
