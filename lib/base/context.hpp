// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CONTEXT_H
#define CONTEXT_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <functional>
#include <vector>

namespace icinga
{

class ContextTrace
{
public:
	ContextTrace();

	void Print(std::ostream& fp) const;

	size_t GetLength() const;

private:
	std::vector<String> m_Frames;
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
	ContextFrame(std::function<void(std::ostream&)> message);
	~ContextFrame();

private:
	static std::vector<std::function<void(std::ostream&)>>& GetFrames();

	friend class ContextTrace;
};

/* The currentContextFrame variable has to be volatile in order to prevent
 * the compiler from optimizing it away. */
#define CONTEXT(message) volatile icinga::ContextFrame currentContextFrame ([&](std::ostream& _CONTEXT_stream) { \
_CONTEXT_stream << message; \
})

}

#endif /* CONTEXT_H */
