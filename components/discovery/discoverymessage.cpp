#include "i2-discovery.h"

using namespace icinga;

DiscoveryMessage::DiscoveryMessage(void)
	: MessagePart()
{ }

DiscoveryMessage::DiscoveryMessage(const MessagePart& message)
	: MessagePart(message)
{ }

bool DiscoveryMessage::GetIdentity(string *value) const
{
	return GetProperty("identity", value);
}

void DiscoveryMessage::SetIdentity(const string& value)
{
	SetProperty("identity", value);
}

bool DiscoveryMessage::GetNode(string *value) const
{
	return GetProperty("node", value);
}

void DiscoveryMessage::SetNode(const string& value)
{
	SetProperty("node", value);
}

bool DiscoveryMessage::GetService(string *value) const
{
	return GetProperty("service", value);
}

void DiscoveryMessage::SetService(const string& value)
{
	SetProperty("service", value);
}

bool DiscoveryMessage::GetSubscriptions(MessagePart *value) const
{
	return GetProperty("subscriptions", value);
}

void DiscoveryMessage::SetSubscriptions(MessagePart value)
{
	SetProperty("subscriptions", value);
}

bool DiscoveryMessage::GetPublications(MessagePart *value) const
{
	return GetProperty("publications", value);
}

void DiscoveryMessage::SetPublications(MessagePart value)
{
	SetProperty("publications", value);
}