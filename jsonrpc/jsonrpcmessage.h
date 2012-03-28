#ifndef I2_JSONRPCMESSAGE_H
#define I2_JSONRPCMESSAGE_H

namespace icinga
{

class JsonRpcMessage : public Object
{
private:
	cJSON *m_JSON;

	string GetFieldString(const char *field);

public:
	typedef shared_ptr<JsonRpcMessage> RefType;
	typedef weak_ptr<JsonRpcMessage> WeakRefType;

	JsonRpcMessage(void);
	~JsonRpcMessage(void);

	static JsonRpcMessage::RefType FromNetstring(Netstring::RefType ns);
	Netstring::RefType ToNetstring(void);

	void SetID(string id);
	string GetID(void);

	void SetMethod(string method);
	string GetMethod(void);

	void SetParams(string params);
	string GetParams(void);

	void SetResult(string result);
	string GetResult(void);

	void SetError(string error);
	string GetError(void);
};

}

#endif /* I2_JSONRPCMESSAGE_H */
