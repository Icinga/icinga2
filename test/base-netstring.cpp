/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
