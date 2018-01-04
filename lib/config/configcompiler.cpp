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

#include "config/configcompiler.hpp"
#include "config/configitem.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/loader.hpp"
#include "base/context.hpp"
#include "base/exception.hpp"
#include <fstream>

using namespace icinga;

std::vector<String> ConfigCompiler::m_IncludeSearchDirs;
boost::mutex ConfigCompiler::m_ZoneDirsMutex;
std::map<String, std::vector<ZoneFragment> > ConfigCompiler::m_ZoneDirs;

/**
 * Constructor for the ConfigCompiler class.
 *
 * @param path The path of the configuration file (or another name that
 *        identifies the source of the configuration text).
 * @param input Input stream for the configuration file.
 * @param zone The zone.
 */
ConfigCompiler::ConfigCompiler(const String& path, std::istream *input,
	const String& zone, const String& package)
	: m_Path(path), m_Input(input), m_Zone(zone), m_Package(package),
	m_Eof(false), m_OpenBraces(0)
{
	InitializeScanner();
}

/**
 * Destructor for the ConfigCompiler class.
 */
ConfigCompiler::~ConfigCompiler()
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
void *ConfigCompiler::GetScanner() const
{
	return m_Scanner;
}

/**
 * Retrieves the path for the input file.
 *
 * @returns The path.
 */
const char *ConfigCompiler::GetPath() const
{
	return m_Path.CStr();
}

void ConfigCompiler::SetZone(const String& zone)
{
	m_Zone = zone;
}

String ConfigCompiler::GetZone() const
{
	return m_Zone;
}

void ConfigCompiler::SetPackage(const String& package)
{
	m_Package = package;
}

String ConfigCompiler::GetPackage() const
{
	return m_Package;
}

void ConfigCompiler::CollectIncludes(std::vector<std::unique_ptr<Expression> >& expressions,
	const String& file, const String& zone, const String& package)
{
	try {
		expressions.emplace_back(CompileFile(file, zone, package));
	} catch (const std::exception& ex) {
		Log(LogWarning, "ConfigCompiler")
			<< "Cannot compile file '"
			<< file << "': " << DiagnosticInformation(ex);
	}
}

/**
 * Handles an include directive.
 *
 * @param relativeBath The path this include is relative to.
 * @param path The path from the include directive.
 * @param search Whether to search global include dirs.
 * @param debuginfo Debug information.
 */
std::unique_ptr<Expression> ConfigCompiler::HandleInclude(const String& relativeBase, const String& path,
	bool search, const String& zone, const String& package, const DebugInfo& debuginfo)
{
	String upath;

	if (search || (IsAbsolutePath(path)))
		upath = path;
	else
		upath = relativeBase + "/" + path;

	String includePath = upath;

	if (search) {
		for (const String& dir : m_IncludeSearchDirs) {
			String spath = dir + "/" + path;

			if (Utility::PathExists(spath)) {
				includePath = spath;
				break;
			}
		}
	}

	std::vector<std::unique_ptr<Expression> > expressions;

	if (!Utility::Glob(includePath, std::bind(&ConfigCompiler::CollectIncludes, std::ref(expressions), _1, zone, package), GlobFile) && includePath.FindFirstOf("*?") == String::NPos) {
		std::ostringstream msgbuf;
		msgbuf << "Include file '" + path + "' does not exist";
		BOOST_THROW_EXCEPTION(ScriptError(msgbuf.str(), debuginfo));
	}

	std::unique_ptr<DictExpression> expr{new DictExpression(std::move(expressions))};
	expr->MakeInline();
	return std::move(expr);
}

/**
 * Handles recursive includes.
 *
 * @param relativeBase The path this include is relative to.
 * @param path The directory path.
 * @param pattern The file pattern.
 * @param debuginfo Debug information.
 */
std::unique_ptr<Expression> ConfigCompiler::HandleIncludeRecursive(const String& relativeBase, const String& path,
	const String& pattern, const String& zone, const String& package, const DebugInfo&)
{
	String ppath;

	if (IsAbsolutePath(path))
		ppath = path;
	else
		ppath = relativeBase + "/" + path;

	std::vector<std::unique_ptr<Expression> > expressions;
	Utility::GlobRecursive(ppath, pattern, std::bind(&ConfigCompiler::CollectIncludes, std::ref(expressions), _1, zone, package), GlobFile);

	std::unique_ptr<DictExpression> dict{new DictExpression(std::move(expressions))};
	dict->MakeInline();
	return std::move(dict);
}

void ConfigCompiler::HandleIncludeZone(const String& relativeBase, const String& tag, const String& path, const String& pattern, const String& package, std::vector<std::unique_ptr<Expression> >& expressions)
{
	String zoneName = Utility::BaseName(path);

	String ppath;

	if (IsAbsolutePath(path))
		ppath = path;
	else
		ppath = relativeBase + "/" + path;

	RegisterZoneDir(tag, ppath, zoneName);

	Utility::GlobRecursive(ppath, pattern, std::bind(&ConfigCompiler::CollectIncludes, std::ref(expressions), _1, zoneName, package), GlobFile);
}

/**
 * Handles zone includes.
 *
 * @param relativeBase The path this include is relative to.
 * @param tag The tag name.
 * @param path The directory path.
 * @param pattern The file pattern.
 * @param debuginfo Debug information.
 */
std::unique_ptr<Expression> ConfigCompiler::HandleIncludeZones(const String& relativeBase, const String& tag,
	const String& path, const String& pattern, const String& package, const DebugInfo&)
{
	String ppath;
	String newRelativeBase = relativeBase;

	if (IsAbsolutePath(path))
		ppath = path;
	else {
		ppath = relativeBase + "/" + path;
		newRelativeBase = ".";
	}

	std::vector<std::unique_ptr<Expression> > expressions;
	Utility::Glob(ppath + "/*", std::bind(&ConfigCompiler::HandleIncludeZone, newRelativeBase, tag, _1, pattern, package, std::ref(expressions)), GlobDirectory);
	return std::unique_ptr<Expression>(new DictExpression(std::move(expressions)));
}

/**
 * Compiles a stream.
 *
 * @param path A name identifying the stream.
 * @param stream The input stream.
 * @returns Configuration items.
 */
std::unique_ptr<Expression> ConfigCompiler::CompileStream(const String& path,
	std::istream *stream, const String& zone, const String& package)
{
	CONTEXT("Compiling configuration stream with name '" + path + "'");

	stream->exceptions(std::istream::badbit);

	ConfigCompiler ctx(path, stream, zone, package);

	try {
		return ctx.Compile();
	} catch (const ScriptError& ex) {
		return std::unique_ptr<Expression>(new ThrowExpression(MakeLiteral(ex.what()), ex.IsIncompleteExpression(), ex.GetDebugInfo()));
	} catch (const std::exception& ex) {
		return std::unique_ptr<Expression>(new ThrowExpression(MakeLiteral(DiagnosticInformation(ex)), false));
	}
}

/**
 * Compiles a file.
 *
 * @param path The path.
 * @returns Configuration items.
 */
std::unique_ptr<Expression> ConfigCompiler::CompileFile(const String& path, const String& zone,
	const String& package)
{
	CONTEXT("Compiling configuration file '" + path + "'");

	std::ifstream stream(path.CStr(), std::ifstream::in);

	if (!stream)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("std::ifstream::open")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));

	Log(LogNotice, "ConfigCompiler")
		<< "Compiling config file: " << path;

	return CompileStream(path, &stream, zone, package);
}

/**
 * Compiles a snippet of text.
 *
 * @param path A name identifying the text.
 * @param text The text.
 * @returns Configuration items.
 */
std::unique_ptr<Expression> ConfigCompiler::CompileText(const String& path, const String& text,
	const String& zone, const String& package)
{
	std::stringstream stream(text);
	return CompileStream(path, &stream, zone, package);
}

/**
 * Adds a directory to the list of include search dirs.
 *
 * @param dir The new dir.
 */
void ConfigCompiler::AddIncludeSearchDir(const String& dir)
{
	Log(LogInformation, "ConfigCompiler")
		<< "Adding include search dir: " << dir;

	m_IncludeSearchDirs.push_back(dir);
}

std::vector<ZoneFragment> ConfigCompiler::GetZoneDirs(const String& zone)
{
	boost::mutex::scoped_lock lock(m_ZoneDirsMutex);
	auto it = m_ZoneDirs.find(zone);
	if (it == m_ZoneDirs.end())
		return std::vector<ZoneFragment>();
	else
		return it->second;
}

void ConfigCompiler::RegisterZoneDir(const String& tag, const String& ppath, const String& zoneName)
{
	ZoneFragment zf;
	zf.Tag = tag;
	zf.Path = ppath;

	boost::mutex::scoped_lock lock(m_ZoneDirsMutex);
	m_ZoneDirs[zoneName].push_back(zf);
}

bool ConfigCompiler::HasZoneConfigAuthority(const String& zoneName)
{
	std::vector<ZoneFragment> zoneDirs = m_ZoneDirs[zoneName];

	bool empty = zoneDirs.empty();

	if (!empty) {
		std::vector<String> paths;
		for (const ZoneFragment& zf : zoneDirs) {
			paths.push_back(zf.Path);
		}

		Log(LogNotice, "ConfigCompiler")
			<< "Registered authoritative config directories for zone '" << zoneName << "': " << Utility::NaturalJoin(paths);
	}

	return !empty;
}


bool ConfigCompiler::IsAbsolutePath(const String& path)
{
#ifndef _WIN32
	return (path.GetLength() > 0 && path[0] == '/');
#else /* _WIN32 */
	return !PathIsRelative(path.CStr());
#endif /* _WIN32 */
}

