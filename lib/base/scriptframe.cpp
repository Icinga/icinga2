/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/scriptframe.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::stack<ScriptFrame *> > ScriptFrame::m_ScriptFrames;

ScriptFrame::ScriptFrame(void)
	: Locals(new Dictionary()), Self(ScriptGlobal::GetGlobals()), Sandboxed(false)
{
	PushFrame(this);
}

ScriptFrame::ScriptFrame(const Value& self)
	: Locals(new Dictionary()), Self(self), Sandboxed(false)
{
	PushFrame(this);
}

ScriptFrame::~ScriptFrame(void)
{
	ScriptFrame *frame = PopFrame();
	ASSERT(frame == this);
}

ScriptFrame *ScriptFrame::GetCurrentFrame(void)
{
	std::stack<ScriptFrame *> *frames = m_ScriptFrames.get();

	ASSERT(!frames->empty());
	return frames->top();
}

ScriptFrame *ScriptFrame::PopFrame(void)
{
	std::stack<ScriptFrame *> *frames = m_ScriptFrames.get();

	ASSERT(!frames->empty());

	ScriptFrame *frame = frames->top();
	frames->pop();

	return frame;
}

void ScriptFrame::PushFrame(ScriptFrame *frame)
{
	std::stack<ScriptFrame *> *frames = m_ScriptFrames.get();

	if (!frames) {
		frames = new std::stack<ScriptFrame *>();
		m_ScriptFrames.reset(frames);
	}

	if (frames->size() > 500)
		BOOST_THROW_EXCEPTION(ScriptError("Recursion level too deep."));

	frames->push(frame);
}
