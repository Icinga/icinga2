// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/stdiostream.hpp"
#include "base/string.hpp"
#include <BoostTestTargetConfig.h>
#include <sstream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_stream)

BOOST_AUTO_TEST_CASE(readline_stdio)
{
	std::stringstream msgbuf;
	msgbuf << "Hello\nWorld\n\n";

	StdioStream::Ptr stdstream = new StdioStream(&msgbuf, false);

	StreamReadContext rlc;

	String line;
	BOOST_CHECK(stdstream->ReadLine(&line, rlc) == StatusNewItem);
	BOOST_CHECK(line == "Hello");

	BOOST_CHECK(stdstream->ReadLine(&line, rlc) == StatusNewItem);
	BOOST_CHECK(line == "World");

	BOOST_CHECK(stdstream->ReadLine(&line, rlc) == StatusNewItem);
	BOOST_CHECK(line == "");

	BOOST_CHECK(stdstream->ReadLine(&line, rlc) == StatusNewItem);
	BOOST_CHECK(line == "");

	BOOST_CHECK(stdstream->ReadLine(&line, rlc) == StatusEof);

	stdstream->Close();
}

BOOST_AUTO_TEST_SUITE_END()
