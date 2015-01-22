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

using namespace icinga;

boost::thread_specific_ptr<ScriptFrame *> ScriptFrame::m_CurrentFrame;

ScriptFrame::ScriptFrame(void)
	: Locals(new Dictionary()), Self(ScriptGlobal::GetGlobals())
{
	NextFrame = GetCurrentFrame();
	SetCurrentFrame(this);
}

ScriptFrame::ScriptFrame(const Value& self)
	: Locals(new Dictionary()), Self(self)
{
	NextFrame = GetCurrentFrame();
	SetCurrentFrame(this);
}

ScriptFrame::~ScriptFrame(void)
{
	ASSERT(GetCurrentFrame() == this);
	SetCurrentFrame(NextFrame);
}

ScriptFrame *ScriptFrame::GetCurrentFrame(void)
{
	ScriptFrame **pframe = m_CurrentFrame.get();

	if (pframe)
		return *pframe;
	else
		return NULL;
}

void ScriptFrame::SetCurrentFrame(ScriptFrame *frame)
{
	m_CurrentFrame.reset(new ScriptFrame *(frame));
}