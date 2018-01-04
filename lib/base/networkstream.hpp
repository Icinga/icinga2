/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef NETWORKSTREAM_H
#define NETWORKSTREAM_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"
#include "base/socket.hpp"

namespace icinga
{

/**
 * A network stream.
 *
 * @ingroup base
 */
class NetworkStream final : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(NetworkStream);

	NetworkStream(const Socket::Ptr& socket);

	size_t Read(void *buffer, size_t count, bool allow_partial = false) override;
	void Write(const void *buffer, size_t count) override;

	void Close() override;

	bool IsEof() const override;

private:
	Socket::Ptr m_Socket;
	bool m_Eof;
};

}

#endif /* NETWORKSTREAM_H */
