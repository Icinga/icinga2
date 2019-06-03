/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STACKTRACE_H
#define STACKTRACE_H

#include "base/i2-base.hpp"
#include <iosfwd>

namespace icinga
{

/**
 * A stacktrace.
 *
 * @ingroup base
 */
class StackTrace
{
public:
	StackTrace();
#ifdef _WIN32
	explicit StackTrace(PEXCEPTION_POINTERS exi);
#endif /* _WIN32 */

	void Print(std::ostream& fp, int ignoreFrames = 0) const;

	static void StaticInitialize();

private:
	void *m_Frames[64];
	int m_Count;
};

std::ostream& operator<<(std::ostream& stream, const StackTrace& trace);

}

#endif /* UTILITY_H */
