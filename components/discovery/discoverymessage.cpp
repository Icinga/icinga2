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
	return Get("identity", value);
}

void DiscoveryMessage::SetIdentity(const string& value)
{
	Set("identity", value);
}

bool DiscoveryMessage::GetNode(string *value) const
{
	return Get("node", value);
}

void DiscoveryMessage::SetNode(const string& value)
{
	Set("node", value);
}

bool DiscoveryMessage::GetService(string *value) const
{
	return Get("service", value);
}

void DiscoveryMessage::SetService(const string& value)
{
	Set("service", value);
}

bool DiscoveryMessage::GetSubscriptions(MessagePart *value) const
{
	return Get("subscriptions", value);
}

void DiscoveryMessage::SetSubscriptions(MessagePart value)
{
	Set("subscriptions", value);
}

bool DiscoveryMessage::GetPublications(MessagePart *value) const
{
	return Get("publications", value);
}

void DiscoveryMessage::SetPublications(MessagePart value)
{
	Set("publications", value);
}
