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

JsonRpcMessage::RefType JsonRpcMessage::FromNetstring(Netstring::RefType ns)
{
	JsonRpcMessage::RefType msg = new_object<JsonRpcMessage>();
	msg->m_JSON = cJSON_Parse(ns->ToString());
	return msg;
}

Netstring::RefType JsonRpcMessage::ToNetstring(void)
{
	return Netstring::RefType();
}

string JsonRpcMessage::GetFieldString(const char *field)
{
	cJSON *idObject = cJSON_GetObjectItem(m_JSON, field);

	if (idObject == NULL || idObject->type != cJSON_String)
		return string();

	return string(idObject->valuestring);
}

void JsonRpcMessage::SetID(string id)
{
}

string JsonRpcMessage::GetID(void)
{
	return GetFieldString("id");
}

void JsonRpcMessage::SetMethod(string method)
{
}

string JsonRpcMessage::GetMethod(void)
{
	return GetFieldString("method");
}

void JsonRpcMessage::SetParams(string params)
{
}

string JsonRpcMessage::GetParams(void)
{
	return GetFieldString("params");
}

void JsonRpcMessage::SetResult(string result)
{
}

string JsonRpcMessage::GetResult(void)
{
	return GetFieldString("result");
}

void JsonRpcMessage::SetError(string error)
{
}

string JsonRpcMessage::GetError(void)
{
	return GetFieldString("error");
}
