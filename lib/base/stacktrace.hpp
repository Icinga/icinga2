/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <boost/stacktrace.hpp>

namespace icinga
{

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
