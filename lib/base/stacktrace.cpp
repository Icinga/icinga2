// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <base/i2-base.hpp>
#include "base/stacktrace.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

#ifdef HAVE_BACKTRACE_SYMBOLS
#	include <execinfo.h>
#endif /* HAVE_BACKTRACE_SYMBOLS */

using namespace icinga;

std::ostream &icinga::operator<<(std::ostream &os, const StackTraceFormatter &f)
{
	/* In most cases, this operator<< just relies on the operator<< for the `boost::stacktrace::stacktrace` wrapped in
	 * the `StackTraceFormatter`. But as this operator turned out to not work properly on some platforms, there is a
	 * fallback implementation that can be enabled using the `-DICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS` flag at
	 * compile time. This will then switch to `backtrace_symbols()` from `<execinfo.h>` instead of the implementation
	 * provided by Boost.
	 */

	const boost::stacktrace::stacktrace &stack = f.m_Stack;

#ifdef ICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS
	std::vector<void *> addrs;
	addrs.reserve(stack.size());
	std::transform(stack.begin(), stack.end(), std::back_inserter(addrs), [](const boost::stacktrace::frame &f) {
		return const_cast<void *>(f.address());
	});

	char **symbols = backtrace_symbols(addrs.data(), addrs.size());
	for (size_t i = 0; i < addrs.size(); i++) {
		os << std::setw(2) << i << "# " << symbols[i] << std::endl;
	}
	std::free(symbols);
#else /* ICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS */
	os << stack;
#endif /* ICINGA2_STACKTRACE_USE_BACKTRACE_SYMBOLS */

	return os;
}
