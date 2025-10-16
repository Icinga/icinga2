/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	globalNS->SetAttribute("System", new ConstEmbeddedNamespaceValue(systemNS));

	systemNS->SetAttribute("Configuration", new EmbeddedNamespaceValue(new Configuration()));

	auto typesNSBehavior = new ConstNamespaceBehavior();
	typesNSBehavior->Freeze();
	Namespace::Ptr typesNS = new Namespace(typesNSBehavior);
	globalNS->SetAttribute("Types", new ConstEmbeddedNamespaceValue(typesNS));

	auto statsNSBehavior = new ConstNamespaceBehavior();
	statsNSBehavior->Freeze();
	Namespace::Ptr statsNS = new Namespace(statsNSBehavior);
	globalNS->SetAttribute("StatsFunctions", new ConstEmbeddedNamespaceValue(statsNS));

	Namespace::Ptr internalNS = new Namespace(l_InternalNSBehavior);
	globalNS->SetAttribute("Internal", new ConstEmbeddedNamespaceValue(internalNS));
}, 1000);

INITIALIZE_ONCE_WITH_PRIORITY([]() {
	l_InternalNSBehavior->Freeze();
}, 0);

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
			for (auto& kv : globals) {
				if (kv.first == "TicketSalt") {
					continue;
				}

				if (kv.first == "Types" || kv.first == "System" || kv.first == "Icinga" || PermChecker->CanAccessGlobalVariable(kv.first)) {
					Globals->Set(kv.first, kv.second->Get());
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
