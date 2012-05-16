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

#ifndef RPCREQUEST_H
#define RPCREQUEST_H

namespace icinga
{

class I2_JSONRPC_API RpcRequest : public MessagePart
{

public:
	RpcRequest(void) : MessagePart() {
		SetVersion("2.0");
	}

	RpcRequest(const MessagePart& message) : MessagePart(message) { }

	inline bool GetVersion(string *value) const
	{
		return GetProperty("jsonrpc", value);
	}

	inline void SetVersion(const string& value)
	{
		SetProperty("jsonrpc", value);
	}

	inline bool GetMethod(string *value) const
	{
		return GetProperty("method", value);
	}

	inline void SetMethod(const string& value)
	{
		SetProperty("method", value);
	}

	inline bool GetParams(MessagePart *value) const
	{
		return GetProperty("params", value);
	}

	inline void SetParams(const MessagePart& value)
	{
		SetProperty("params", value);
	}

	inline bool GetID(string *value) const
	{
		return GetProperty("id", value);
	}

	inline void SetID(const string& value)
	{
		SetProperty("id", value);
	}
};

}

#endif /* RPCREQUEST_H */
