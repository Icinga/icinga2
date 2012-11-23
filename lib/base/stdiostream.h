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

#ifndef STDIOSTREAM_H
#define STDIOSTREAM_H

namespace icinga {

class StdioStream : public Stream
{
public:
	typedef shared_ptr<StdioStream> Ptr;
	typedef weak_ptr<StdioStream> WeakPtr;

	StdioStream(iostream *innerStream, bool ownsStream);
	~StdioStream(void);

	virtual void Start(void);

	virtual size_t GetAvailableBytes(void) const;
	virtual size_t Read(void *buffer, size_t size);
	virtual size_t Peek(void *buffer, size_t size);
	virtual void Write(const void *buffer, size_t size);

	virtual void Close(void);

private:
	iostream *m_InnerStream;
	bool m_OwnsStream;
	FIFO::Ptr m_ReadAheadBuffer;
};

}

#endif /* STDIOSTREAM_H */