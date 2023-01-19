/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/scriptframe.hpp"
#include "base/scriptglobal.hpp"
#include "base/namespace.hpp"
#include "base/exception.hpp"
#include "base/configuration.hpp"
#include "base/utility.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::stack<ScriptFrame *> > ScriptFrame::m_ScriptFrames;

static Namespace::Ptr l_SystemNS, l_TypesNS, l_StatsNS, l_InternalNS;

/* Ensure that this gets called with highest priority
 * and wins against other static initializers in lib/icinga, etc.
 * LTO-enabled builds will cause trouble otherwise, see GH #6575.
 */
INITIALIZE_ONCE_WITH_PRIORITY([]() {
	Namespace::Ptr globalNS = ScriptGlobal::GetGlobals();

	l_SystemNS = new Namespace(true);
	l_SystemNS->Set("PlatformKernel", Utility::GetPlatformKernel());
	l_SystemNS->Set("PlatformKernelVersion", Utility::GetPlatformKernelVersion());
	l_SystemNS->Set("PlatformName", Utility::GetPlatformName());
	l_SystemNS->Set("PlatformVersion", Utility::GetPlatformVersion());
	l_SystemNS->Set("PlatformArchitecture", Utility::GetPlatformArchitecture());
	l_SystemNS->Set("BuildHostName", ICINGA_BUILD_HOST_NAME);
	l_SystemNS->Set("BuildCompilerName", ICINGA_BUILD_COMPILER_NAME);
	l_SystemNS->Set("BuildCompilerVersion", ICINGA_BUILD_COMPILER_VERSION);
	globalNS->Set("System", l_SystemNS, true);

	l_SystemNS->Set("Configuration", new Configuration());

	l_TypesNS = new Namespace(true);
	globalNS->Set("Types", l_TypesNS, true);

	l_StatsNS = new Namespace(true);
	globalNS->Set("StatsFunctions", l_StatsNS, true);

	l_InternalNS = new Namespace(true);
	globalNS->Set("Internal", l_InternalNS, true);
}, InitializePriority::CreateNamespaces);

INITIALIZE_ONCE_WITH_PRIORITY([]() {
	l_SystemNS->Freeze();
	l_TypesNS->Freeze();
	l_StatsNS->Freeze();
	l_InternalNS->Freeze();
}, InitializePriority::FreezeNamespaces);

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

#ifndef I2_DEBUG
	(void)frame;
#endif /* I2_DEBUG */
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
