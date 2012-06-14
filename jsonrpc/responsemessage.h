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

#ifndef RESPONSEMESSAGE_H
#define RESPONSEMESSAGE_H

namespace icinga
{

/**
 * A JSON-RPC response message.
 *
 * @ingroup jsonrpc
 */
class I2_JSONRPC_API ResponseMessage : public MessagePart
{
public:
	/**
	 * Constructor for the ResponseMessage class.
	 */
	ResponseMessage(void) : MessagePart() {
		SetVersion("2.0");
	}

	/**
	 * Copy-constructor for the ResponseMessage class.
	 *
	 * @param message The message that should be copied.
	 */
	ResponseMessage(const MessagePart& message) : MessagePart(message) { }

	/**
	 * Retrieves the version of the JSON-RPC protocol.
	 *
	 * @param[out] value The value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	inline bool GetVersion(string *value) const
	{
		return GetProperty("jsonrpc", value);
	}

	/**
	 * Sets the version of the JSON-RPC protocol that should be used.
	 *
	 * @param value The version.
	 */
	inline void SetVersion(const string& value)
	{
		SetProperty("jsonrpc", value);
	}

	/**
	 * Retrieves the result of the JSON-RPC call.
	 *
	 * @param[out] value The result.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	bool GetResult(MessagePart *value) const
	{
		return GetProperty("result", value);
	}

	/**
	 * Sets the result for the JSON-RPC call.
	 *
	 * @param value The result.
	 */
	void SetResult(const MessagePart& value)
	{
		SetProperty("result", value);
	}

	/**
	 * Retrieves the error message of the JSON-RPC call.
	 *
	 * @param[out] value The error message.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	bool GetError(string *value) const
	{
		return GetProperty("error", value);
	}

	/**
	 * Sets the error message for the JSON-RPC call.
	 *
	 * @param value The error message.
	 */
	void SetError(const string& value)
	{
		SetProperty("error", value);
	}

	/**
	 * Retrieves the ID of the JSON-RPC call.
	 *
	 * @param[out] value The ID.
	 * @return true if the value was retrieved, false otherwise.
	 */
	bool GetID(string *value) const
	{
		return GetProperty("id", value);
	}

	/**
	 * Sets the ID for the JSON-RPC call.
	 *
	 * @param value The ID.
	 */
	void SetID(const string& value)
	{
		SetProperty("id", value);
	}

	/**
	 * Checks whether a message is a response message.
	 *
	 * @param message The message.
	 * @returns true if the message is a response message, false otherwise.
	 */
	static bool IsResponseMessage(const MessagePart& message)
	{
		return (message.Contains("result"));
	}
};

}

#endif /* RESPONSEMESSAGE_H */
