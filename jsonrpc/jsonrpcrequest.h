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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef JSONRPCREQUEST_H
#define JSONRPCREQUEST_H

namespace icinga
{

class I2_JSONRPC_API JsonRpcRequest : public Message
{

public:
	JsonRpcRequest(void) : Message() {
		SetVersion("2.0");
	}

	JsonRpcRequest(const Message& message) : Message(message) { }

	inline bool GetVersion(string *value) const
	{
		return GetPropertyString("jsonrpc", value);
	}

	inline void SetVersion(const string& value)
	{
		SetPropertyString("jsonrpc", value);
	}

	inline bool GetMethod(string *value) const
	{
		return GetPropertyString("method", value);
	}

	inline void SetMethod(const string& value)
	{
		SetPropertyString("method", value);
	}

	inline bool GetParams(Message *value) const
	{
		return GetPropertyMessage("params", value);
	}

	inline void SetParams(const Message& value)
	{
		SetPropertyMessage("params", value);
	}

	inline bool GetID(string *value) const
	{
		return GetPropertyString("id", value);
	}

	inline void SetID(const string& value)
	{
		SetPropertyString("id", value);
	}
};

}

#endif /* JSONRPCREQUEST_H */
