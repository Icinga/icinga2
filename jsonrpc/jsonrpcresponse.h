#ifndef JSONRPCRESPONSE_H
#define JSONRPCRESPONSE_H

namespace icinga
{

class I2_JSONRPC_API JsonRpcResponse : public Message
{
public:
	JsonRpcResponse(void) : Message() { }
	JsonRpcResponse(const Message& message) : Message(message) { }

	inline bool GetVersion(string *value) const
	{
		return GetDictionary()->GetValueString("jsonrpc", value);
	}

	inline void SetJsonRpc(const string& value)
	{
		GetDictionary()->SetValueString("jsonrpc", value);
	}

	bool GetResult(string *value) const
	{
		return GetDictionary()->GetValueString("result", value);
	}

	void SetResult(const string& value)
	{
		GetDictionary()->SetValueString("result", value);
	}

	bool GetError(string *value) const
	{
		return GetDictionary()->GetValueString("error", value);
	}

	void SetError(const string& value)
	{
		GetDictionary()->SetValueString("error", value);
	}

	bool GetID(string *value) const
	{
		return GetDictionary()->GetValueString("id", value);
	}

	void SetID(const string& value)
	{
		GetDictionary()->SetValueString("id", value);
	}
};

}

#endif /* JSONRPCRESPONSE_H */
