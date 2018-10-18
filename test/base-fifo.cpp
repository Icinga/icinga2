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

#include "base/fifo.hpp"
#include "base/objectlock.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_fifo)

BOOST_AUTO_TEST_CASE(construct)
{
	FIFO::Ptr fifo = new FIFO();
	BOOST_CHECK(fifo);
	BOOST_CHECK(fifo->GetAvailableBytes() == 0);

	fifo->Close();
}

BOOST_AUTO_TEST_CASE(io)
{
	FIFO::Ptr fifo = new FIFO();

	fifo->Write("hello", 5);
	BOOST_CHECK(fifo->GetAvailableBytes() == 5);

	char buffer1[2];
	fifo->Read(buffer1, 2, true);
	BOOST_CHECK(memcmp(buffer1, "he", 2) == 0);
	BOOST_CHECK(fifo->GetAvailableBytes() == 3);

	char buffer2[5];
	size_t rc = fifo->Read(buffer2, 5, true);
	BOOST_CHECK(rc == 3);
	BOOST_CHECK(memcmp(buffer2, "llo", 3) == 0);
	BOOST_CHECK(fifo->GetAvailableBytes() == 0);

	BOOST_CHECK(!fifo->IsEof());

	fifo->Close();
}

BOOST_AUTO_TEST_SUITE_END()
