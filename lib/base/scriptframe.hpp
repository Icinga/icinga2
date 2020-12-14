/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include <boost/thread/tss.hpp>
#include <stack>

namespace icinga
{

struct ScriptFrame
{
	Dictionary::Ptr Locals;
	Value Self;
	bool Sandboxed;
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
