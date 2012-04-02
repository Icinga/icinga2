#ifndef JSONRPCMESSAGE_H
#define JSONRPCMESSAGE_H

namespace icinga
{

class JsonRpcMessage : public Object
{
private:
	cJSON *m_JSON;

	void InitJson(void);

	void SetFieldString(const char *field, const string& value);
	string GetFieldString(const char *field);

	void ClearField(const char *field);
	void SetFieldObject(const char *field, cJSON *object);
	cJSON *GetFieldObject(const char *field);

public:
	typedef shared_ptr<JsonRpcMessage> Ptr;
	typedef weak_ptr<JsonRpcMessage> WeakPtr;

	JsonRpcMessage(void);
	~JsonRpcMessage(void);

	void SetJSON(cJSON *object);
	cJSON *GetJSON(void);
	
	void SetVersion(const string& version);
	string GetVersion(void);

	void SetID(const string& id);
	string GetID(void);

	void SetMethod(const string& method);
	string GetMethod(void);

	void ClearParams(void);
	cJSON *GetParams(void);

	void SetParam(const string& name, const string& value);
	cJSON *GetParam(const string& name);
	bool GetParamString(const string name, string *value);

	void ClearResult();
	cJSON *GetResult(void);

	void SetError(const string& error);
	string GetError(void);
};

}

#endif /* JSONRPCMESSAGE_H */
