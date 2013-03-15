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

#include "i2-base.h"
#include <boost/bind.hpp>

using namespace icinga;

Connection::Connection(const Stream::Ptr& stream)
	: m_Stream(stream)
{
	m_Stream->OnDataAvailable.connect(boost::bind(&Connection::ProcessData, this));
	m_Stream->OnClosed.connect(boost::bind(&Connection::ClosedHandler, this));
}

Stream::Ptr Connection::GetStream(void) const
{
	return m_Stream;
}

void Connection::ClosedHandler(void)
{
	OnClosed(GetSelf());
}

void Connection::Close(void)
{
	m_Stream->Close();
}
