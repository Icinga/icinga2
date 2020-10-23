/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

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
