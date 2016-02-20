/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include <boost/foreach.hpp>

using namespace icinga;

std::vector<String> ConfigCompiler::m_IncludeSearchDirs;
boost::mutex ConfigCompiler::m_ZoneDirsMutex;
std::map<String, std::vector<ZoneFragment> > ConfigCompiler::m_ZoneDirs;

/**
 * Constructor for the ConfigCompiler class.
 *
 * @param path The path of the configuration file (or another name that
 *	       identifies the source of the configuration text).
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
const char *ConfigCompiler::GetPath(void) const
{
	return m_Path.CStr();
}

void ConfigCompiler::SetZone(const String& zone)
{
	m_Zone = zone;
}

String ConfigCompiler::GetZone(void) const
{
	return m_Zone;
}

void ConfigCompiler::SetPackage(const String& package)
{
	m_Package = package;
}

String ConfigCompiler::GetPackage(void) const
{
	return m_Package;
}

void ConfigCompiler::CollectIncludes(std::vector<Expression *>& expressions,
    const String& file, const String& zone, const String& package)
{
	expressions.push_back(CompileFile(file, zone, package));
}

/**
 * Handles an include directive.
 *
 * @param relativeBath The path this include is relative to.
 * @param path The path from the include directive.
 * @param search Whether to search global include dirs.
 * @param debuginfo Debug information.
 */
Expression *ConfigCompiler::HandleInclude(const String& relativeBase, const String& path,
    bool search, const String& zone, const String& package, const DebugInfo& debuginfo)
{
	String upath;

	if (search || (path.GetLength() > 0 && path[0] == '/'))
		upath = path;
	else
		upath = relativeBase + "/" + path;

	String includePath = upath;

	if (search) {
		BOOST_FOREACH(const String& dir, m_IncludeSearchDirs) {
			String spath = dir + "/" + path;

			if (Utility::PathExists(spath)) {
				includePath = spath;
				break;
			}
		}
	}

	std::vector<Expression *> expressions;

	if (!Utility::Glob(includePath, boost::bind(&ConfigCompiler::CollectIncludes, boost::ref(expressions), _1, zone, package), GlobFile) && includePath.FindFirstOf("*?") == String::NPos) {
		std::ostringstream msgbuf;
		msgbuf << "Include file '" + path + "' does not exist";
		BOOST_THROW_EXCEPTION(ScriptError(msgbuf.str(), debuginfo));
	}

	DictExpression *expr = new DictExpression(expressions);
	expr->MakeInline();
	return expr;
}

/**
 * Handles recursive includes.
 *
 * @param relativeBase The path this include is relative to.
 * @param path The directory path.
 * @param pattern The file pattern.
 * @param debuginfo Debug information.
 */
Expression *ConfigCompiler::HandleIncludeRecursive(const String& relativeBase, const String& path,
    const String& pattern, const String& zone, const String& package, const DebugInfo&)
{
	String ppath;

	if (path.GetLength() > 0 && path[0] == '/')
		ppath = path;
	else
		ppath = relativeBase + "/" + path;

	std::vector<Expression *> expressions;
	Utility::GlobRecursive(ppath, pattern, boost::bind(&ConfigCompiler::CollectIncludes, boost::ref(expressions), _1, zone, package), GlobFile);
	return new DictExpression(expressions);
}

void ConfigCompiler::HandleIncludeZone(const String& relativeBase, const String& tag, const String& path, const String& pattern, const String& package, std::vector<Expression *>& expressions)
{
	String zoneName = Utility::BaseName(path);

	String ppath;

	if (path.GetLength() > 0 && path[0] == '/')
		ppath = path;
	else
		ppath = relativeBase + "/" + path;

	RegisterZoneDir(tag, ppath, zoneName);

	Utility::GlobRecursive(ppath, pattern, boost::bind(&ConfigCompiler::CollectIncludes, boost::ref(expressions), _1, zoneName, package), GlobFile);
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
Expression *ConfigCompiler::HandleIncludeZones(const String& relativeBase, const String& tag,
    const String& path, const String& pattern, const String& package, const DebugInfo&)
{
	String ppath;
	String newRelativeBase = relativeBase;

	if (path.GetLength() > 0 && path[0] == '/')
		ppath = path;
	else {
		ppath = relativeBase + "/" + path;
		newRelativeBase = ".";
	}

	std::vector<Expression *> expressions;
	Utility::Glob(ppath + "/*", boost::bind(&ConfigCompiler::HandleIncludeZone, newRelativeBase, tag, _1, pattern, package, boost::ref(expressions)), GlobDirectory);
	return new DictExpression(expressions);
}

/**
 * Compiles a stream.
 *
 * @param path A name identifying the stream.
 * @param stream The input stream.
 * @returns Configuration items.
 */
Expression *ConfigCompiler::CompileStream(const String& path,
    std::istream *stream, const String& zone, const String& package)
{
	CONTEXT("Compiling configuration stream with name '" + path + "'");

	stream->exceptions(std::istream::badbit);

	ConfigCompiler ctx(path, stream, zone, package);

	try {
		return ctx.Compile();
	} catch (const ScriptError& ex) {
		return new ThrowExpression(MakeLiteral(ex.what()), ex.IsIncompleteExpression(), ex.GetDebugInfo());
	} catch (const std::exception& ex) {
		return new ThrowExpression(MakeLiteral(DiagnosticInformation(ex)), false);
	}
}

/**
 * Compiles a file.
 *
 * @param path The path.
 * @returns Configuration items.
 */
Expression *ConfigCompiler::CompileFile(const String& path, const String& zone,
    const String& package)
{
	CONTEXT("Compiling configuration file '" + path + "'");

	std::ifstream stream(path.CStr(), std::ifstream::in);

	if (!stream)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("std::ifstream::open")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));

	Log(LogInformation, "ConfigCompiler")
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
Expression *ConfigCompiler::CompileText(const String& path, const String& text,
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
	std::map<String, std::vector<ZoneFragment> >::const_iterator it = m_ZoneDirs.find(zone);
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
		BOOST_FOREACH(const ZoneFragment& zf, zoneDirs) {
			paths.push_back(zf.Path);
		}

		Log(LogNotice, "ConfigCompiler")
		    << "Registered authoritative config directories for zone '" << zoneName << "': " << Utility::NaturalJoin(paths);
	}

	return !empty;
}

