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

#include "i2-config.h"

using std::ifstream;

using namespace icinga;

vector<String> ConfigCompiler::m_IncludeSearchDirs;

/**
 * Constructor for the ConfigCompiler class.
 *
 * @param path The path of the configuration file (or another name that
 *	       identifies the source of the configuration text).
 * @param input Input stream for the configuration file.
 * @param includeHandler Handler function for #include directives.
 */
ConfigCompiler::ConfigCompiler(const String& path, istream *input,
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
 * Retrieves the result from the compiler.
 *
 * @returns A list of configuration items.
 */
vector<ConfigItem::Ptr> ConfigCompiler::GetResultObjects(void) const
{
	return m_ResultObjects;
}

/**
 * Retrieves the resulting type objects from the compiler.
 *
 * @returns A list of type objects.
 */
vector<ConfigType::Ptr> ConfigCompiler::GetResultTypes(void) const
{
	vector<ConfigType::Ptr> types;
	
	ConfigType::Ptr type;
	BOOST_FOREACH(tie(tuples::ignore, type), m_ResultTypes) {
		types.push_back(type);
	}
	
	return types;
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
 */
void ConfigCompiler::HandleInclude(const String& include, bool search)
{
	String path = Utility::DirName(GetPath()) + "/" + include;

	vector<ConfigType::Ptr> types;
	m_HandleInclude(path, search, &m_ResultObjects, &types);

	BOOST_FOREACH(const ConfigType::Ptr& type, types) {
		AddType(type);
	}
}

/**
 * Handles the library directive.
 *
 * @param library The name of the library.
 */
void ConfigCompiler::HandleLibrary(const String& library)
{
	Utility::LoadIcingaLibrary(library, false);
}

/**
 * Compiles a stream.
 *
 * @param path A name identifying the stream.
 * @param stream The input stream.
 * @returns Configuration items.
 */
void ConfigCompiler::CompileStream(const String& path,
    istream *stream, vector<ConfigItem::Ptr> *resultItems, vector<ConfigType::Ptr> *resultTypes)
{
	stream->exceptions(istream::badbit);

	ConfigCompiler ctx(path, stream);
	ctx.Compile();
	
	if (resultItems) {
		vector<ConfigItem::Ptr> items = ctx.GetResultObjects();
		std::copy(items.begin(), items.end(), std::back_inserter(*resultItems));
	}

	if (resultTypes) {	
		vector<ConfigType::Ptr> types = ctx.GetResultTypes();
		std::copy(types.begin(), types.end(), std::back_inserter(*resultTypes));
	}
}

/**
 * Compiles a file.
 *
 * @param path The path.
 * @returns Configuration items.
 */
void ConfigCompiler::CompileFile(const String& path,
    vector<ConfigItem::Ptr> *resultItems, vector<ConfigType::Ptr> *resultTypes)
{
	ifstream stream;
	stream.open(path.CStr(), ifstream::in);

	if (!stream)
		throw_exception(invalid_argument("Could not open config file: " + path));

	Logger::Write(LogInformation, "config", "Compiling config file: " + path);

	return CompileStream(path, &stream, resultItems, resultTypes);
}

/**
 * Compiles a snippet of text.
 *
 * @param path A name identifying the text.
 * @param text The text.
 * @returns Configuration items.
 */
void ConfigCompiler::CompileText(const String& path, const String& text,
    vector<ConfigItem::Ptr> *resultItems, vector<ConfigType::Ptr> *resultTypes)
{
	stringstream stream(text);
	return CompileStream(path, &stream, resultItems, resultTypes);
}

/**
 * Default include handler. Includes the file and returns a list of
 * configuration items.
 *
 * @param include The path from the include directive.
 */
void ConfigCompiler::HandleFileInclude(const String& include, bool search,
    vector<ConfigItem::Ptr> *resultItems, vector<ConfigType::Ptr> *resultTypes)
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

	vector<ConfigItem::Ptr> items;

	if (!Utility::Glob(includePath, boost::bind(&ConfigCompiler::CompileFile, _1, resultItems, resultTypes)))
		throw_exception(invalid_argument("Include file '" + include + "' does not exist (or no files found for pattern)."));
}

/**
 * Adds an object to the result.
 *
 * @param object The configuration item.
 */
void ConfigCompiler::AddObject(const ConfigItem::Ptr& object)
{
	m_ResultObjects.push_back(object);
}

/**
 * Adds a directory to the list of include search dirs.
 *
 * @param dir The new dir.
 */
void ConfigCompiler::AddIncludeSearchDir(const String& dir)
{
	Logger::Write(LogInformation, "config", "Adding include search dir: " + dir);

	m_IncludeSearchDirs.push_back(dir);
}

void ConfigCompiler::AddType(const ConfigType::Ptr& type)
{
	m_ResultTypes[type->GetName()] = type;
}

ConfigType::Ptr ConfigCompiler::GetTypeByName(const String& name) const
{
	map<String, ConfigType::Ptr>::const_iterator it;
	
	it = m_ResultTypes.find(name);
	
	if (it == m_ResultTypes.end())
		return ConfigType::Ptr();
	
	return it->second;
}
