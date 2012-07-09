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

#ifndef REQUESTMESSAGE_H
#define REQUESTMESSAGE_H

namespace icinga
{

/**
 * A JSON-RPC request message.
 *
 * @ingroup jsonrpc
 */
class I2_JSONRPC_API RequestMessage : public MessagePart
{
public:
	/**
	 * Constructor for the RequestMessage class.
	 */
	RequestMessage(void) : MessagePart() {
		SetVersion("2.0");
	}

	/**
	 * Copy-constructor for the RequestMessage class.
	 *
	 * @param message The message that is to be copied.
	 */
	RequestMessage(const MessagePart& message) : MessagePart(message) { }

	/**
	 * Retrieves the version of the JSON-RPC protocol.
	 *
	 * @param[out] value The value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	inline bool GetVersion(string *value) const
	{
		return Get("jsonrpc", value);
	}

	/**
	 * Sets the version of the JSON-RPC protocol that should be used.
	 *
	 * @param value The version.
	 */
	inline void SetVersion(const string& value)
	{
		Set("jsonrpc", value);
	}

	/**
	 * Retrieves the method of the JSON-RPC call.
	 *
	 * @param[out] value The method.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	inline bool GetMethod(string *value) const
	{
		return Get("method", value);
	}

	/**
	 * Sets the method for the JSON-RPC call.
	 *
	 * @param value The method.
	 */
	inline void SetMethod(const string& value)
	{
		Set("method", value);
	}

	/**
	 * Retrieves the parameters of the JSON-RPC call.
	 *
	 * @param[out] value The parameters.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	inline bool GetParams(MessagePart *value) const
	{
		return Get("params", value);
	}

	/**
	 * Sets the parameters for the JSON-RPC call.
	 *
	 * @param value The parameters.
	 */
	inline void SetParams(const MessagePart& value)
	{
		Set("params", value);
	}

	/**
	 * Retrieves the ID of the JSON-RPC call.
	 *
	 * @param[out] value The ID.
	 * @return true if the value was retrieved, false otherwise.
	 */
	inline bool GetID(string *value) const
	{
		return Get("id", value);
	}

	/**
	 * Sets the ID for the JSON-RPC call.
	 *
	 * @param value The ID.
	 */
	inline void SetID(const string& value)
	{
		Set("id", value);
	}
};

}

#endif /* REQUESTMESSAGE_H */
