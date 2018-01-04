/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
	ContextFrame(const String& message);
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
