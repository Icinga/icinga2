/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef STREAM_H
#define STREAM_H

#include "base/i2-base.hpp"
#include "base/object.hpp"

namespace icinga
{

class String;

enum ConnectionRole
{
	RoleClient,
	RoleServer
};

struct ReadLineContext
{
	ReadLineContext(void) : Buffer(NULL), Size(0), Eof(false), MustRead(true)
	{ }

	~ReadLineContext(void)
	{
		free(Buffer);
	}

	char *Buffer;
	size_t Size;
	bool Eof;
	bool MustRead;
};

/**
 * A stream.
 *
 * @ingroup base
 */
class I2_BASE_API Stream : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Stream);

	/**
	 * Reads data from the stream.
	 *
	 * @param buffer The buffer where data should be stored. May be NULL if you're
	 *		 not actually interested in the data.
	 * @param count The number of bytes to read from the queue.
	 * @returns The number of bytes actually read.
	 */
	virtual size_t Read(void *buffer, size_t count) = 0;

	/**
	 * Writes data to the stream.
	 *
	 * @param buffer The data that is to be written.
	 * @param count The number of bytes to write.
	 * @returns The number of bytes written
	 */
	virtual void Write(const void *buffer, size_t count) = 0;

	/**
	 * Closes the stream and releases resources.
	 */
	virtual void Close(void) = 0;

	/**
	 * Checks whether we've reached the end-of-file condition.
	 *
	 * @returns true if EOF.
	 */
	virtual bool IsEof(void) const = 0;

	bool ReadLine(String *line, ReadLineContext& context);
};

}

#endif /* STREAM_H */
