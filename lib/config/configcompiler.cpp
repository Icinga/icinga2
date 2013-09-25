/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "config/configcompiler.h"
#include "config/configitem.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include <sstream>
#include <fstream>
#include <boost/foreach.hpp>

using std::ifstream;

using namespace icinga;

std::vector<String> ConfigCompiler::m_IncludeSearchDirs;

/**
 * Constructor for the ConfigCompiler class.
 *
 * @param path The path of the configuration file (or another name that
 *	       identifies the source of the configuration text).
 * @param input Input stream for the configuration file.
 * @param includeHandler Handler function for #include directives.
 */
ConfigCompiler::ConfigCompiler(const String& path, std::istream *input,
    HandleIncludeFunc includeHandler)
	: m_Path(path), m_Input(input), m_HandleInclude(includeHandler)
{
	InitializeScanner();
}

/**
 * Destructor for the ConfigCompiler class.
 */
ConfigCompiler::~ConfigCompiler(void)
{
	DestroyScanner();
}

/**
 * Reads data from the input stream. Used internally by the lexer.
 *
 * @param buffer Where to store data.
 * @param max_size The maximum number of bytes to read from the stream.
 * @returns The actual number of bytes read.
 */
size_t ConfigCompiler::ReadInput(char *buffer, size_t max_size)
{
	m_Input->read(buffer, max_size);
	return static_cast<size_t>(m_Input->gcount());
}

/**
 * Retrieves the scanner object.
 *
 * @returns The scanner object.
 */
void *ConfigCompiler::GetScanner(void) const
{
	return m_Scanner;
}

/**
 * Retrieves the path for the input file.
 *
 * @returns The path.
 */
String ConfigCompiler::GetPath(void) const
{
	return m_Path;
}

/**
 * Handles an include directive by calling the include handler callback
 * function.
 *
 * @param include The path from the include directive.
 * @param search Whether to search global include dirs.
 * @param debuginfo Debug information.
 */
void ConfigCompiler::HandleInclude(const String& include, bool search, const DebugInfo& debuginfo)
{
	String path;

	if (search || (include.GetLength() > 0 && include[0] == '/'))
		path = include;
	else
		path = Utility::DirName(GetPath()) + "/" + include;

	m_HandleInclude(path, search, debuginfo);
}

/**
 * Handles the library directive.
 *
 * @param library The name of the library.
 */
void ConfigCompiler::HandleLibrary(const String& library)
{
	(void) Utility::LoadExtensionLibrary(library);
}

/**
 * Compiles a stream.
 *
 * @param path A name identifying the stream.
 * @param stream The input stream.
 * @returns Configuration items.
 */
void ConfigCompiler::CompileStream(const String& path, std::istream *stream)
{
	stream->exceptions(std::istream::badbit);

	ConfigCompiler ctx(path, stream);
	ctx.Compile();
}

/**
 * Compiles a file.
 *
 * @param path The path.
 * @returns Configuration items.
 */
void ConfigCompiler::CompileFile(const String& path)
{
	std::ifstream stream;
	stream.open(path.CStr(), std::ifstream::in);

	if (!stream)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Could not open config file: " + path));

	Log(LogInformation, "config", "Compiling config file: " + path);

	return CompileStream(path, &stream);
}

/**
 * Compiles a snippet of text.
 *
 * @param path A name identifying the text.
 * @param text The text.
 * @returns Configuration items.
 */
void ConfigCompiler::CompileText(const String& path, const String& text)
{
	std::stringstream stream(text);
	return CompileStream(path, &stream);
}

/**
 * Default include handler. Includes the file and returns a list of
 * configuration items.
 *
 * @param include The path from the include directive.
 * @param search Whether to search include dirs.
 * @param debuginfo Debug information.
 */
void ConfigCompiler::HandleFileInclude(const String& include, bool search,
    const DebugInfo& debuginfo)
{
	String includePath = include;

	if (search) {
		String path;

		BOOST_FOREACH(const String& dir, m_IncludeSearchDirs) {
			String path = dir + "/" + include;

#ifndef _WIN32
			struct stat statbuf;
			if (lstat(path.CStr(), &statbuf) >= 0) {
#else /* _WIN32 */
			struct _stat statbuf;
			if (_stat(path.CStr(), &statbuf) >= 0) {
#endif /* _WIN32 */
				includePath = path;
				break;
			}
		}
	}

	std::vector<ConfigItem::Ptr> items;

	if (!Utility::Glob(includePath, boost::bind(&ConfigCompiler::CompileFile, _1)) && includePath.FindFirstOf("*?") == String::NPos) {
		std::ostringstream msgbuf;
		msgbuf << "Include file '" + include + "' does not exist: " << debuginfo;
		BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
	}
}

/**
 * Adds a directory to the list of include search dirs.
 *
 * @param dir The new dir.
 */
void ConfigCompiler::AddIncludeSearchDir(const String& dir)
{
	Log(LogInformation, "config", "Adding include search dir: " + dir);

	m_IncludeSearchDirs.push_back(dir);
}
