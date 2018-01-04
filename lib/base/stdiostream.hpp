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

#ifndef STDIOSTREAM_H
#define STDIOSTREAM_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"
#include <iostream>

namespace icinga {

class StdioStream final : public Stream
{
public:
	DECLARE_PTR_TYPEDEFS(StdioStream);

	StdioStream(std::iostream *innerStream, bool ownsStream);
	~StdioStream(void);

	virtual size_t Read(void *buffer, size_t size, bool allow_partial = false) override;
	virtual void Write(const void *buffer, size_t size) override;

	virtual void Close(void) override;

	virtual bool IsDataAvailable(void) const override;
	virtual bool IsEof(void) const override;

private:
	std::iostream *m_InnerStream;
	bool m_OwnsStream;
};

}

#endif /* STDIOSTREAM_H */
