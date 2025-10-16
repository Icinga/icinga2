/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SCRIPTFRAME_H
#define SCRIPTFRAME_H

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include "base/namespace.hpp"
#include "base/scriptpermission.hpp"
#include <boost/thread/tss.hpp>
#include <stack>

namespace icinga
{

/**
 * A frame describing the context a section of script code is executed in.
 *
 * This is implemented by each new object that is constructed getting pushed on a thread_local
 * global stack that is accessible from anywhere during script evaluation.
 *
 * Most properties in this frame, like local variables do not carry over to successive frames,
 * except the `PermChecker` and `Sandboxed` members, which get propagated to enforce access
 * control and availability of unsafe functions.
 */
struct ScriptFrame
{
	Dictionary::Ptr Locals;
	ScriptPermissionChecker::Ptr PermChecker; /* inherited by next frame */
	Value Self;
	bool Sandboxed; /* inherited by next frame */
	int Depth;

	ScriptFrame(bool allocLocals);
	ScriptFrame(bool allocLocals, Value self);
	~ScriptFrame();

	void IncreaseStackDepth();
	void DecreaseStackDepth();

	static ScriptFrame *GetCurrentFrame();

	Namespace::Ptr GetGlobals();

private:
	/**
	 * Caches a sanitized version of the global namespace for the current `ScriptFrame`.
	 *
	 * This is a value that is dependent on a ScriptFrame's `Sandboxed` and `CheckPerms`
	 * members. These are both independent of each other and while they are inherited by
	 * subsequent frames themselves, their values can be changed for new frames easily.
	 * Therefore Globals can hold a different value for each ScriptFrame and is not
	 * inherited.
	 */
	Namespace::Ptr Globals;

	static boost::thread_specific_ptr<std::stack<ScriptFrame *> > m_ScriptFrames;

	static void PushFrame(ScriptFrame *frame);
	static ScriptFrame *PopFrame();

	void InitializeFrame();
};

}

#endif /* SCRIPTFRAME_H */
