/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/context.hpp"
#include <boost/thread/tss.hpp>
#include <iostream>

using namespace icinga;

static boost::thread_specific_ptr<std::list<String> > l_Frames;

ContextFrame::ContextFrame(const String& message)
{
	GetFrames().push_front(message);
}

ContextFrame::~ContextFrame()
{
	GetFrames().pop_front();
}

std::list<String>& ContextFrame::GetFrames()
{
	if (!l_Frames.get())
		l_Frames.reset(new std::list<String>());

	return *l_Frames;
}

ContextTrace::ContextTrace()
	: m_Frames(ContextFrame::GetFrames())
{ }

void ContextTrace::Print(std::ostream& fp) const
{
	if (m_Frames.empty())
		return;

	fp << "\n";

	int i = 0;
	for (const auto& frame : m_Frames) {
		fp << "\t(" << i << ") " << frame << "\n";
		i++;
	}
}

size_t ContextTrace::GetLength() const
{
	return m_Frames.size();
}

std::ostream& icinga::operator<<(std::ostream& stream, const ContextTrace& trace)
{
	trace.Print(stream);
	return stream;
}
