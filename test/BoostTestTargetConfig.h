// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// We unconditionally use the included version of Boost.Test now.
#define BOOST_TEST_USE_INCLUDED

// This works around a bug in Boost.Test on boost versions <= 1.93 (presumably, because it hasn't yet been released with
// the upstream fix): https://redirect.github.com/boostorg/test/issues/495
// The problem occurs when it calls `sigaltstack()` without taking the minimum stack size into account, which leads to
// errors like `Test setup error: system_error produced by: exp: Out of memory`. Currently this only affects musl
// systems, so Alpine, but nothing is stopping glibc from adding checks like this in the future. And given that we will
// support targets with older boost versions for quite a while the simplest solution to avoid this issue is turning off
// the alternate stack entirely. For us this has barely any downsides, because the one thing the alt stack would get us
// is recovery if a test case causes a stack overflow, but we don't support running the entire test suite through
// directly through the test binaries anyway, but either run a single test case directly (for debugging in gdb for
// example) or the entire suite via `ctest`.
#define BOOST_TEST_DISABLE_ALT_STACK

// Only one file needs to include the implementation files.
#ifdef TEST_INCLUDE_IMPLEMENTATION
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif
