/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SCRIPTFRAME_H
#define SCRIPTFRAME_H

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include "base/scriptpermission.hpp"
#include <boost/thread/tss.hpp>
#include <stack>

namespace icinga
{

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

private:
	static boost::thread_specific_ptr<std::stack<ScriptFrame *> > m_ScriptFrames;

	static void PushFrame(ScriptFrame *frame);
	static ScriptFrame *PopFrame();

	void InitializeFrame();
};

}

#endif /* SCRIPTFRAME_H */
