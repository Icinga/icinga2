/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include <boost/thread/tss.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

static boost::thread_specific_ptr<std::list<String> > l_Frames;

ContextFrame::ContextFrame(const String& message)
{
	GetFrames().push_front(message);
}

ContextFrame::~ContextFrame(void)
{
	GetFrames().pop_front();
}

std::list<String>& ContextFrame::GetFrames(void)
{
	if (l_Frames.get() == NULL)
		l_Frames.reset(new std::list<String>());

	return *l_Frames;
}

ContextTrace::ContextTrace(void)
	: m_Frames(ContextFrame::GetFrames())
{ }

void ContextTrace::Print(std::ostream& fp) const
{
	fp << std::endl;

	int i = 0;
	BOOST_FOREACH(const String& frame, m_Frames) {
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
