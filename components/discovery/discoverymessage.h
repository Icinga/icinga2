#ifndef DISCOVERYMESSAGE_H
#define DISCOVERYMESSAGE_H

namespace icinga
{

class DiscoveryMessage : public MessagePart
{

public:
	DiscoveryMessage(void) : MessagePart() { }
	DiscoveryMessage(const MessagePart& message) : MessagePart(message) { }

	inline bool GetIdentity(string *value) const
	{
		return GetProperty("identity", value);
	}

	inline void SetIdentity(const string& value)
	{
		SetProperty("identity", value);
	}

	inline bool GetNode(string *value) const
	{
		return GetProperty("node", value);
	}

	inline void SetNode(const string& value)
	{
		SetProperty("node", value);
	}

	inline bool GetService(string *value) const
	{
		return GetProperty("service", value);
	}

	inline void SetService(const string& value)
	{
		SetProperty("service", value);
	}

	inline bool GetSubscriptions(MessagePart *value) const
	{
		return GetProperty("subscriptions", value);
	}

	inline void SetSubscriptions(MessagePart value)
	{
		SetProperty("subscriptions", value);
	}

	inline bool GetPublications(MessagePart *value) const
	{
		return GetProperty("publications", value);
	}

	inline void SetPublications(MessagePart value)
	{
		SetProperty("publications", value);
	}
};

}

#endif /* SUBSCRIPTIONMESSAGE_H */
