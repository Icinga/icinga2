/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include "base/namespace.hpp"
#include "base/exception.hpp"
#include "base/configuration.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::stack<ScriptFrame *> > ScriptFrame::m_ScriptFrames;

static auto l_InternalNSBehavior = new ConstNamespaceBehavior();

/* Ensure that this gets called with highest priority
 * and wins against other static initializers in lib/icinga, etc.
 * LTO-enabled builds will cause trouble otherwise, see GH #6575.
 */
INITIALIZE_ONCE_WITH_PRIORITY([]() {
	Namespace::Ptr globalNS = ScriptGlobal::GetGlobals();

	auto systemNSBehavior = new ConstNamespaceBehavior();
	systemNSBehavior->Freeze();
	Namespace::Ptr systemNS = new Namespace(systemNSBehavior);
	globalNS->SetAttribute("System", std::make_shared<ConstEmbeddedNamespaceValue>(systemNS));

	systemNS->SetAttribute("Configuration", std::make_shared<EmbeddedNamespaceValue>(new Configuration()));

	auto typesNSBehavior = new ConstNamespaceBehavior();
	typesNSBehavior->Freeze();
	Namespace::Ptr typesNS = new Namespace(typesNSBehavior);
	globalNS->SetAttribute("Types", std::make_shared<ConstEmbeddedNamespaceValue>(typesNS));

	auto statsNSBehavior = new ConstNamespaceBehavior();
	statsNSBehavior->Freeze();
	Namespace::Ptr statsNS = new Namespace(statsNSBehavior);
	globalNS->SetAttribute("StatsFunctions", std::make_shared<ConstEmbeddedNamespaceValue>(statsNS));

	Namespace::Ptr internalNS = new Namespace(l_InternalNSBehavior);
	globalNS->SetAttribute("Internal", std::make_shared<ConstEmbeddedNamespaceValue>(internalNS));
}, 1000);

INITIALIZE_ONCE_WITH_PRIORITY([]() {
	l_InternalNSBehavior->Freeze();
}, 0);

ScriptFrame::ScriptFrame(bool allocLocals)
	: Locals(allocLocals ? new Dictionary() : nullptr), Self(ScriptGlobal::GetGlobals()), Sandboxed(false), Depth(0)
{
	InitializeFrame();
}

ScriptFrame::ScriptFrame(bool allocLocals, Value self)
	: Locals(allocLocals ? new Dictionary() : nullptr), Self(std::move(self)), Sandboxed(false), Depth(0)
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
