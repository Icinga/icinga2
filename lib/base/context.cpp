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

#include "base/context.hpp"
#include <ostream>

using namespace icinga;

thread_local std::list<String> l_ContextFrames;

ContextFrame::ContextFrame(const String& message)
{
	l_ContextFrames.push_front(message);
}

ContextFrame::~ContextFrame(void)
{
	l_ContextFrames.pop_front();
}

ContextTrace::ContextTrace(void)
	: m_Frames(l_ContextFrames)
{ }

void ContextTrace::Print(std::ostream& fp) const
{
	if (m_Frames.empty())
		return;

	fp << "\n";

	int i = 0;
	for (const String& frame : m_Frames) {
		fp << "\t(" << i << ") " << frame << std::endl;
		i++;
	}
}

size_t ContextTrace::GetLength(void) const
{
	return m_Frames.size();
}

std::ostream& icinga::operator<<(std::ostream& stream, const ContextTrace& trace)
{
	trace.Print(stream);
	return stream;
}
