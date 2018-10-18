/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/netstring.hpp"
#include "base/fifo.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_netstring)

BOOST_AUTO_TEST_CASE(netstring)
{
	FIFO::Ptr fifo = new FIFO();

	NetString::WriteStringToStream(fifo, "hello");

	String s;
	StreamReadContext src;
	BOOST_CHECK(NetString::ReadStringFromStream(fifo, &s, src) == StatusNewItem);
	BOOST_CHECK(s == "hello");

	fifo->Close();
}

BOOST_AUTO_TEST_SUITE_END()
