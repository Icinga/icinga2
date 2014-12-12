/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef VMFRAME_H
#define VMFRAME_H

#include "config/i2-config.hpp"
#include "base/dictionary.hpp"
#include <boost/thread/tss.hpp>

namespace icinga
{

struct I2_BASE_API ScriptFrame
{
	Dictionary::Ptr Locals;
	Value Self;
	ScriptFrame *NextFrame;

	ScriptFrame(void)
		: Locals(new Dictionary()), Self(Locals)
	{
		NextFrame = GetCurrentFrame();
		SetCurrentFrame(this);
	}

	ScriptFrame(const Value& self)
		: Locals(new Dictionary()), Self(self)
	{
		NextFrame = GetCurrentFrame();
		SetCurrentFrame(this);
	}

	~ScriptFrame(void)
	{
		ASSERT(GetCurrentFrame() == this);
		SetCurrentFrame(NextFrame);
	}

	static inline ScriptFrame *GetCurrentFrame(void)
	{
		ScriptFrame **pframe = m_CurrentFrame.get();

		if (pframe)
			return *pframe;
		else
			return NULL;
	}

private:
	static boost::thread_specific_ptr<ScriptFrame *> m_CurrentFrame;

	static inline void SetCurrentFrame(ScriptFrame *frame)
	{
		m_CurrentFrame.reset(new ScriptFrame *(frame));
	}
};

}

#endif /* SCRIPTFRAME_H */
