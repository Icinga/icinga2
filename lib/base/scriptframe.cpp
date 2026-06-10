// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/scriptframe.hpp"
#include "base/scriptglobal.hpp"
#include "base/namespace.hpp"
#include "base/exception.hpp"
#include "base/configuration.hpp"
#include "base/utility.hpp"

using namespace icinga;

boost::thread_specific_ptr<std::stack<ScriptFrame *> > ScriptFrame::m_ScriptFrames;

static Namespace::Ptr l_SystemNS, l_StatsNS;

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

	l_StatsNS = new Namespace(true);
	globalNS->Set("StatsFunctions", l_StatsNS, true);

	Namespace::Ptr intNS = new Namespace();
	intNS->Set("modified_attributes", new Dictionary(), true);
	globalNS->Set("Internal", intNS, true);
}, InitializePriority::CreateNamespaces);

INITIALIZE_ONCE_WITH_PRIORITY([]() {
	l_SystemNS->Freeze();
	l_StatsNS->Freeze();
}, InitializePriority::FreezeNamespaces);

/**
 * Construct a @c ScriptFrame that has `Self` assigned to the global namespace.
 *
 * Prefer the other constructor if possible since if misused this may leak global variables
 * without permissions or senstive variables like TicketSalt in a sandboxed context.
 *
 * @todo Remove this constructor and call the other with the global namespace in places where it's actually necessary.
 */
ScriptFrame::ScriptFrame(bool allocLocals)
	: Locals(allocLocals ? new Dictionary() : nullptr), PermChecker(new ScriptPermissionChecker),
	  Self(ScriptGlobal::GetGlobals()), Sandboxed(false), Depth(0), Globals(nullptr)
{
	InitializeFrame();
}

ScriptFrame::ScriptFrame(bool allocLocals, Value self)
	: Locals(allocLocals ? new Dictionary() : nullptr), PermChecker(new ScriptPermissionChecker), Self(std::move(self)),
	  Sandboxed(false), Depth(0), Globals(nullptr)
{
	InitializeFrame();
}

void ScriptFrame::InitializeFrame()
{
	std::stack<ScriptFrame *> *frames = m_ScriptFrames.get();

	if (frames && !frames->empty()) {
		ScriptFrame *frame = frames->top();

		// See the documentation of `ScriptFrame::Globals` for why these two are inherited and Globals isn't.
		PermChecker = frame->PermChecker;
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

/**
 * Returns a sanitized copy of the global variables namespace when sandboxed.
 *
 * This filters out the TicketSalt variable specifically and any variable for which the
 * PermChecker does not return 'true'.
 *
 * However it specifically keeps the Types, System, and Icinga sub-namespaces, because they're
 * accessed through globals in ScopeExpression and the user should have access to all Values
 * contained in these namespaces.
 *
 * @return a sanitized copy of the global namespace if sandboxed, a pointer to the global namespace otherwise.
 */
Namespace::Ptr ScriptFrame::GetGlobals()
{
	if (Sandboxed) {
		if (!Globals) {
			Globals = new Namespace;
			auto globals = ScriptGlobal::GetGlobals();
			ObjectLock lock{globals};
			for (auto& [key, val] : globals) {
				if (key == "TicketSalt") {
					continue;
				}

				if (key == "Types" || key == "System" || key == "Icinga" || PermChecker->CanAccessGlobalVariable(key)) {
					Globals->Set(key, val.Val, val.Const);
				}
			}
		}
		return Globals;
	}

	return ScriptGlobal::GetGlobals();
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
