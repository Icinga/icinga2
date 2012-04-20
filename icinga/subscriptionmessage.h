#ifndef SUBSCRIPTIONMESSAGE_H
#define SUBSCRIPTIONMESSAGE_H

namespace icinga
{

class I2_ICINGA_API SubscriptionMessage : public Message
{

public:
	SubscriptionMessage(void) : Message() { }
	SubscriptionMessage(const Message& message) : Message(message) { }

	inline bool GetMethod(string *value) const
	{
		return GetDictionary()->GetValueString("method", value);
	}

	inline void SetMethod(const string& value)
	{
		GetDictionary()->SetValueString("method", value);
	}
};

}

#endif /* SUBSCRIPTIONMESSAGE_H */
