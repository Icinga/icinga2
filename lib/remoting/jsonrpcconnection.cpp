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

#include "i2-remoting.h"

using namespace icinga;

/**
 * Constructor for the JsonRpcConnection class.
 *
 * @param stream The stream.
 */
JsonRpcConnection::JsonRpcConnection(const Stream::Ptr& stream)
	: Connection(stream)
{ }

/**
 * Sends a message to the connected peer.
 *
 * @param message The message.
 */
void JsonRpcConnection::SendMessage(const MessagePart& message)
{
	Value value = message.GetDictionary();
	String json = value.Serialize();
	//std::cerr << ">> " << json << std::endl;
	NetString::WriteStringToStream(GetStream(), json);
}

/**
 * Processes inbound data.
 */
void JsonRpcConnection::ProcessData(void)
{
	String jsonString;

	while (NetString::ReadStringFromStream(GetStream(), &jsonString)) {
		//std::cerr << "<< " << jsonString << std::endl;

		try {
			Value value = Value::Deserialize(jsonString);

			if (!value.IsObjectType<Dictionary>()) {
				throw_exception(invalid_argument("JSON-RPC"
				    " message must be a dictionary."));
			}

			OnNewMessage(GetSelf(), MessagePart(value));
		} catch (const exception& ex) {
			Logger::Write(LogCritical, "remoting", "Exception"
			    " while processing message from JSON-RPC client: " +
			    String(ex.what()));
		}
	}
}
