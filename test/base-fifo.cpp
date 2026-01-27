// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	fifo->Read(buffer1, 2);
	BOOST_CHECK(memcmp(buffer1, "he", 2) == 0);
	BOOST_CHECK(fifo->GetAvailableBytes() == 3);

	char buffer2[5];
	size_t rc = fifo->Read(buffer2, 5);
	BOOST_CHECK(rc == 3);
	BOOST_CHECK(memcmp(buffer2, "llo", 3) == 0);
	BOOST_CHECK(fifo->GetAvailableBytes() == 0);

	BOOST_CHECK(!fifo->IsEof());

	fifo->Close();
}

BOOST_AUTO_TEST_SUITE_END()
