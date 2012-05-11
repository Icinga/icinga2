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

#ifndef NETSTRING_H
#define NETSTRING_H

struct cJSON;

namespace icinga
{

typedef ::cJSON json_t;

class I2_JSONRPC_API Netstring : public Object
{
private:
	size_t m_Length;
	void *m_Data;

	static Dictionary::Ptr GetDictionaryFromJson(json_t *json);
	static json_t *GetJsonFromDictionary(const Dictionary::Ptr& dictionary);

public:
	typedef shared_ptr<Netstring> Ptr;
	typedef weak_ptr<Netstring> WeakPtr;

	static bool ReadMessageFromFIFO(FIFO::Ptr fifo, Message *message);
	static void WriteMessageToFIFO(FIFO::Ptr fifo, const Message& message);
};

}

#endif /* NETSTRING_H */
