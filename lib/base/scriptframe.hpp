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

#ifndef SCRIPTFRAME_H
#define SCRIPTFRAME_H

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

	static Array::Ptr GetImports();
	static void AddImport(const Object::Ptr& import);

private:
	static boost::thread_specific_ptr<std::stack<ScriptFrame *> > m_ScriptFrames;
	static Array::Ptr m_Imports;

	static void PushFrame(ScriptFrame *frame);
	static ScriptFrame *PopFrame();

	void InitializeFrame();
};

}

#endif /* SCRIPTFRAME_H */
