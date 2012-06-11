/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-dyn.h"

using namespace icinga;

ConfigCompiler::ConfigCompiler(istream *input)
{
	m_Input = input;
	InitializeScanner();
}

ConfigCompiler::~ConfigCompiler(void)
{
	DestroyScanner();
}

size_t ConfigCompiler::ReadInput(char *buffer, size_t max_size)
{
	m_Input->read(buffer, max_size);
	return m_Input->gcount();
}

void *ConfigCompiler::GetScanner(void) const
{
	return m_Scanner;
}

void ConfigCompiler::SetResult(vector<ConfigItem::Ptr> result)
{
	m_Result = result;
}

vector<ConfigItem::Ptr> ConfigCompiler::GetResult(void) const
{
        return m_Result;
}

vector<ConfigItem::Ptr> ConfigCompiler::CompileStream(istream *stream)
{
	ConfigCompiler ctx(stream);
	ctx.Compile();
	return ctx.GetResult();
}

vector<ConfigItem::Ptr> ConfigCompiler::CompileFile(const string& filename)
{
	ifstream stream;
	stream.open(filename, ifstream::in);
	return CompileStream(&stream);
}

vector<ConfigItem::Ptr> ConfigCompiler::CompileText(const string& text)
{
	stringstream stream(text);
	return CompileStream(&stream);
}
