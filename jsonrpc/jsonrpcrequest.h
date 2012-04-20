#ifndef JSONRPCREQUEST_H
#define JSONRPCREQUEST_H

namespace icinga
{

class I2_JSONRPC_API JsonRpcRequest : public Message
{

public:
	JsonRpcRequest(void) : Message() { }
	JsonRpcRequest(const Message& message) : Message(message) { }

	inline bool GetVersion(string *value) const
	{
		return GetPropertyString("jsonrpc", value);
	}

	inline void SetVersion(const string& value)
	{
		SetPropertyString("jsonrpc", value);
	}

	inline bool GetMethod(string *value) const
	{
		return GetPropertyString("method", value);
	}

	inline void SetMethod(const string& value)
	{
		SetPropertyString("method", value);
	}

	inline bool GetParams(Message *value) const
	{
		return GetPropertyMessage("params", value);
	}

	inline void SetParams(const Message& value)
	{
		SetPropertyMessage("params", value);
	}

	inline bool GetID(string *value) const
	{
		return GetPropertyString("id", value);
	}

	inline void SetID(const string& value)
	{
		SetPropertyString("id", value);
	}
};

}

#endif /* JSONRPCREQUEST_H */
