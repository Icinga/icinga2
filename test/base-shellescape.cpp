// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/utility.hpp"
#include <BoostTestTargetConfig.h>
#include <iostream>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_shellescape)

BOOST_AUTO_TEST_CASE(escape_basic)
{
#ifdef _WIN32
	BOOST_CHECK(Utility::EscapeShellCmd("%PATH%") == "^%PATH^%");
#else /* _WIN32 */
	BOOST_CHECK(Utility::EscapeShellCmd("$PATH") == "\\$PATH");
	BOOST_CHECK(Utility::EscapeShellCmd("\\$PATH") == "\\\\\\$PATH");
#endif /* _WIN32 */
}

BOOST_AUTO_TEST_CASE(escape_quoted)
{
#ifdef _WIN32
	BOOST_CHECK(Utility::EscapeShellCmd("'hello'") == "^'hello^'");
	BOOST_CHECK(Utility::EscapeShellCmd("\"hello\"") == "^\"hello^\"");
#else /* _WIN32 */
	BOOST_CHECK(Utility::EscapeShellCmd("'hello'") == "'hello'");
	BOOST_CHECK(Utility::EscapeShellCmd("'hello") == "\\'hello");
#endif /* _WIN32 */
}

BOOST_AUTO_TEST_SUITE_END()
