/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <list>

namespace icinga
{

class ContextTrace
{
public:
	ContextTrace();

	void Print(std::ostream& fp) const;

	size_t GetLength() const;

private:
	std::list<String> m_Frames;
};

std::ostream& operator<<(std::ostream& stream, const ContextTrace& trace);

/**
 * A context frame.
 *
 * @ingroup base
 */
class ContextFrame
{
public:
	ContextFrame(String message);
	~ContextFrame();

private:
	static std::list<String>& GetFrames();

	friend class ContextTrace;
};

/* The currentContextFrame variable has to be volatile in order to prevent
 * the compiler from optimizing it away. */
#define CONTEXT(message) volatile icinga::ContextFrame currentContextFrame(message)
}

#endif /* CONTEXT_H */
