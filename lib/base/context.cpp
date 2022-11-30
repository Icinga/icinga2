/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/context.hpp"
#include <boost/thread/tss.hpp>
#include <iostream>
#include <sstream>
#include <utility>

using namespace icinga;

static boost::thread_specific_ptr<std::vector<std::function<void(std::ostream&)>>> l_Frames;

ContextFrame::ContextFrame(std::function<void(std::ostream&)> message)
{
	GetFrames().emplace_back(std::move(message));
}

ContextFrame::~ContextFrame()
{
	GetFrames().pop_back();
}

std::vector<std::function<void(std::ostream&)>>& ContextFrame::GetFrames()
{
	if (!l_Frames.get())
		l_Frames.reset(new std::vector<std::function<void(std::ostream&)>>());

	return *l_Frames;
}

ContextTrace::ContextTrace()
{
	for (auto frame (ContextFrame::GetFrames().rbegin()); frame != ContextFrame::GetFrames().rend(); ++frame) {
		std::ostringstream oss;

		(*frame)(oss);
		m_Frames.emplace_back(oss.str());
	}
}

void ContextTrace::Print(std::ostream& fp) const
{
	if (m_Frames.empty())
		return;

	fp << "\n";

	int i = 0;
	for (const String& frame : m_Frames) {
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
