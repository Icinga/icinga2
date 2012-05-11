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

#ifndef JSONRPCRESPONSE_H
#define JSONRPCRESPONSE_H

namespace icinga
{

class I2_JSONRPC_API JsonRpcResponse : public Message
{
public:
	JsonRpcResponse(void) : Message() {
		SetVersion("2.0");
	}

	JsonRpcResponse(const Message& message) : Message(message) { }

	inline bool GetVersion(string *value) const
	{
		return GetPropertyString("jsonrpc", value);
	}

	inline void SetVersion(const string& value)
	{
		SetPropertyString("jsonrpc", value);
	}

	bool GetResult(string *value) const
	{
		return GetPropertyString("result", value);
	}

	void SetResult(const string& value)
	{
		SetPropertyString("result", value);
	}

	bool GetError(string *value) const
	{
		return GetPropertyString("error", value);
	}

	void SetError(const string& value)
	{
		SetPropertyString("error", value);
	}

	bool GetID(string *value) const
	{
		return GetPropertyString("id", value);
	}

	void SetID(const string& value)
	{
		SetPropertyString("id", value);
	}
};

}

#endif /* JSONRPCRESPONSE_H */
