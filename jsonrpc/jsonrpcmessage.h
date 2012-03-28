#ifndef I2_JSONRPCMESSAGE_H
#define I2_JSONRPCMESSAGE_H

namespace icinga
{

class JsonRpcMessage : public Object
{
private:
	cJSON *m_JSON;

	void SetFieldString(const char *field, const string& value);
	string GetFieldString(const char *field);

public:
	typedef shared_ptr<JsonRpcMessage> RefType;
	typedef weak_ptr<JsonRpcMessage> WeakRefType;

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

	void SetParams(const string& params);
	string GetParams(void);

	void SetResult(const string& result);
	string GetResult(void);

	void SetError(const string& error);
	string GetError(void);
};

}

#endif /* I2_JSONRPCMESSAGE_H */
