// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// We unconditionally use the included version of Boost.Test now.
#define BOOST_TEST_USE_INCLUDED

// Only one file needs to include the implementation files.
#ifdef TEST_INCLUDE_IMPLEMENTATION
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif
