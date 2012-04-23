#ifndef JSONRPCRESPONSE_H
#define JSONRPCRESPONSE_H

namespace icinga
{

class I2_JSONRPC_API JsonRpcResponse : public Message
{
public:
	JsonRpcResponse(void) : Message() {
		SetVersion("2.0");
	}

	JsonRpcResponse(const Message& message) : Message(message) { }

	inline bool GetVersion(string *value) const
	{
		return GetPropertyString("jsonrpc", value);
	}

	inline void SetVersion(const string& value)
	{
		SetPropertyString("jsonrpc", value);
	}

	bool GetResult(string *value) const
	{
		return GetPropertyString("result", value);
	}

	void SetResult(const string& value)
	{
		SetPropertyString("result", value);
	}

	bool GetError(string *value) const
	{
		return GetPropertyString("error", value);
	}

	void SetError(const string& value)
	{
		SetPropertyString("error", value);
	}

	bool GetID(string *value) const
	{
		return GetPropertyString("id", value);
	}

	void SetID(const string& value)
	{
		SetPropertyString("id", value);
	}
};

}

#endif /* JSONRPCRESPONSE_H */
