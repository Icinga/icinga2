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
		return GetDictionary()->GetValueString("jsonrpc", value);
	}

	inline void SetVersion(const string& value)
	{
		GetDictionary()->SetValueString("jsonrpc", value);
	}

	inline bool GetMethod(string *value) const
	{
		return GetDictionary()->GetValueString("method", value);
	}

	inline void SetMethod(const string& value)
	{
		GetDictionary()->SetValueString("method", value);
	}

	inline bool GetParams(Message *value) const
	{
		Dictionary::Ptr dictionary;

		if (!GetDictionary()->GetValueDictionary("params", &dictionary))
			return false;

		*value = Message(dictionary);

		return true;
	}

	inline void SetParams(const Message& value)
	{
		GetDictionary()->SetValueDictionary("params", value.GetDictionary());
	}

	inline bool GetID(string *value) const
	{
		return GetDictionary()->GetValueString("id", value);
	}

	inline void SetID(const string& value)
	{
		GetDictionary()->SetValueString("id", value);
	}
};

}

#endif /* JSONRPCREQUEST_H */
