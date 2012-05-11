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

#ifndef MESSAGE_H
#define MESSAGE_H

namespace icinga
{

class I2_JSONRPC_API Message
{
private:
	Dictionary::Ptr m_Dictionary;

public:
	Message(void);
	Message(const Dictionary::Ptr& dictionary);
	Message(const Message& message);

	Dictionary::Ptr GetDictionary(void) const;

	bool GetPropertyString(string key, string *value) const;
	void SetPropertyString(string key, const string& value);

	bool GetPropertyInteger(string key, long *value) const;
	void SetPropertyInteger(string key, long value);

	bool GetPropertyMessage(string key, Message *value) const;
	void SetPropertyMessage(string key, const Message& value);

	void AddUnnamedPropertyString(const string& value);
	void AddUnnamedPropertyInteger(long value);
	void AddUnnamedPropertyMessage(const Message& value);
};

}

#endif /* MESSAGE_H */
