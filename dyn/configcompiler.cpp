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

using std::ifstream;

using namespace icinga;

ConfigCompiler::ConfigCompiler(istream *input, HandleIncludeFunc includeHandler)
{
	m_HandleInclude = includeHandler;
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
	return static_cast<size_t>(m_Input->gcount());
}

void *ConfigCompiler::GetScanner(void) const
{
	return m_Scanner;
}

vector<ConfigItem::Ptr> ConfigCompiler::GetResult(void) const
{
        return m_Result;
}

void ConfigCompiler::HandleInclude(const string& include)
{
	vector<ConfigItem::Ptr> items = m_HandleInclude(include);
	std::copy(items.begin(), items.end(), back_inserter(m_Result));
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
	stream.exceptions(ifstream::badbit);
	stream.open(filename.c_str(), ifstream::in);

	if (!stream.good())
		throw invalid_argument("Could not open config file: " + filename);

	Application::Log(LogInformation, "dyn", "Compiling config file: " + filename);

	return CompileStream(&stream);
}

vector<ConfigItem::Ptr> ConfigCompiler::CompileText(const string& text)
{
	stringstream stream(text);
	return CompileStream(&stream);
}

vector<ConfigItem::Ptr> ConfigCompiler::HandleFileInclude(const string& include)
{
	/* TODO: implement wildcard includes */
	return CompileFile(include);
}

void ConfigCompiler::AddObject(const ConfigItem::Ptr& object)
{
	m_Result.push_back(object);
}
