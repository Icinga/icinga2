#ifndef DISCOVERYMESSAGE_H
#define DISCOVERYMESSAGE_H

namespace icinga
{

/**
 * @ingroup discovery
 */
class DiscoveryMessage : public MessagePart
{
public:
	DiscoveryMessage(void);
	DiscoveryMessage(const MessagePart& message);

	bool GetIdentity(string *value) const;
	void SetIdentity(const string& value);

	bool GetNode(string *value) const;
	void SetNode(const string& value);

	bool GetService(string *value) const;
	void SetService(const string& value);

	bool GetSubscriptions(MessagePart *value) const;
	void SetSubscriptions(MessagePart value);

	bool GetPublications(MessagePart *value) const;
	void SetPublications(MessagePart value);
};

}

#endif /* SUBSCRIPTIONMESSAGE_H */
