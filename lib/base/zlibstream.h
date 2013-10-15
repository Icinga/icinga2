/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef ZLIBSTREAM_H
#define ZLIBSTREAM_H

#include "base/i2-base.h"
#include "base/stream_bio.h"
#include <iostream>

#ifdef HAVE_BIOZLIB

namespace icinga {

class I2_BASE_API ZlibStream : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(ZlibStream);

	ZlibStream(const Stream::Ptr& innerStream);
	~ZlibStream(void);

	virtual size_t Read(void *buffer, size_t size);
	virtual void Write(const void *buffer, size_t size);

	virtual void Close(void);

	virtual bool IsEof(void) const;

private:
	Stream::Ptr m_InnerStream;
	BIO *m_BIO;
};

}

#endif /* HAVE_BIOZLIB */

#endif /* ZLIBSTREAM_H */
