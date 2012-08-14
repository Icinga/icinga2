/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-discovery.h"

using namespace icinga;

DiscoveryMessage::DiscoveryMessage(void)
	: MessagePart()
{ }

DiscoveryMessage::DiscoveryMessage(const MessagePart& message)
	: MessagePart(message)
{ }

bool DiscoveryMessage::GetIdentity(String *value) const
{
	return Get("identity", value);
}

void DiscoveryMessage::SetIdentity(const String& value)
{
	Set("identity", value);
}

bool DiscoveryMessage::GetNode(String *value) const
{
	return Get("node", value);
}

void DiscoveryMessage::SetNode(const String& value)
{
	Set("node", value);
}

bool DiscoveryMessage::GetService(String *value) const
{
	return Get("service", value);
}

void DiscoveryMessage::SetService(const String& value)
{
	Set("service", value);
}

bool DiscoveryMessage::GetSubscriptions(Dictionary::Ptr *value) const
{
	return Get("subscriptions", value);
}

void DiscoveryMessage::SetSubscriptions(const Dictionary::Ptr& value)
{
	Set("subscriptions", value);
}

