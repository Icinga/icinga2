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

#include "base/scriptframe.hpp"
#include "base/scriptglobal.hpp"
#include "base/exception.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::stack<ScriptFrame *> > ScriptFrame::m_ScriptFrames;
Array::Ptr ScriptFrame::m_Imports;

INITIALIZE_ONCE_WITH_PRIORITY([]() {
	Dictionary::Ptr systemNS = new Dictionary();
	ScriptGlobal::Set("System", systemNS);
	ScriptFrame::AddImport(systemNS);

	Dictionary::Ptr typesNS = new Dictionary();
	ScriptGlobal::Set("Types", typesNS);
	ScriptFrame::AddImport(typesNS);

	Dictionary::Ptr deprecatedNS = new Dictionary();
	ScriptGlobal::Set("Deprecated", deprecatedNS);
	ScriptFrame::AddImport(deprecatedNS);
}, 50);

ScriptFrame::ScriptFrame(bool allocLocals)
	: Locals(allocLocals ? new Dictionary() : nullptr), Self(ScriptGlobal::GetGlobals()), Sandboxed(false), Depth(0)
{
	InitializeFrame();
}

ScriptFrame::ScriptFrame(bool allocLocals, const Value& self)
	: Locals(allocLocals ? new Dictionary() : nullptr), Self(self), Sandboxed(false), Depth(0)
{
	InitializeFrame();
}

void ScriptFrame::InitializeFrame()
{
	std::stack<ScriptFrame *> *frames = m_ScriptFrames.get();

	if (frames && !frames->empty()) {
		ScriptFrame *frame = frames->top();

		Sandboxed = frame->Sandboxed;
	}

	PushFrame(this);
}

ScriptFrame::~ScriptFrame()
{
	ScriptFrame *frame = PopFrame();
	ASSERT(frame == this);
}

void ScriptFrame::IncreaseStackDepth()
{
	if (Depth + 1 > 300)
		BOOST_THROW_EXCEPTION(ScriptError("Stack overflow while evaluating expression: Recursion level too deep."));

	Depth++;
}

void ScriptFrame::DecreaseStackDepth()
{
	Depth--;
}

ScriptFrame *ScriptFrame::GetCurrentFrame()
{
	std::stack<ScriptFrame *> *frames = m_ScriptFrames.get();

	ASSERT(!frames->empty());
	return frames->top();
}

ScriptFrame *ScriptFrame::PopFrame()
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

	if (!frames->empty()) {
		ScriptFrame *parent = frames->top();
		frame->Depth += parent->Depth;
	}

	frames->push(frame);
}

Array::Ptr ScriptFrame::GetImports()
{
	return m_Imports;
}

void ScriptFrame::AddImport(const Object::Ptr& import)
{
	Array::Ptr imports;

	if (!m_Imports)
		imports = new Array();
	else
		imports = m_Imports->ShallowClone();

	imports->Add(import);

	m_Imports = imports;
}

