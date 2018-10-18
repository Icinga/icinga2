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
