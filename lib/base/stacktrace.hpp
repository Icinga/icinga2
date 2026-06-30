// SPDX-FileCopyrightText: 2020 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <boost/stacktrace.hpp>

namespace icinga
{

/**
 * Formatter for `boost::stacktrace::stacktrace` objects
 *
 * This class wraps `boost::stacktrace::stacktrace` objects and provides an operator<<
 * for printing them to an `std::ostream` in a custom format.
 */
class StackTraceFormatter {
public:
	StackTraceFormatter(const boost::stacktrace::stacktrace &stack) : m_Stack(stack) {}

private:
	const boost::stacktrace::stacktrace &m_Stack;

	friend std::ostream &operator<<(std::ostream &os, const StackTraceFormatter &f);
};

std::ostream& operator<<(std::ostream& os, const StackTraceFormatter &f);

}

#endif /* STACKTRACE_H */
