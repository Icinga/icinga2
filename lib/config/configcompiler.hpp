/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef CONFIGCOMPILER_H
#define CONFIGCOMPILER_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"
#include "base/registry.hpp"
#include "base/initialize.hpp"
#include "base/singleton.hpp"
#include <boost/function.hpp>
#include <iostream>
#include <stack>

typedef union YYSTYPE YYSTYPE;
typedef void *yyscan_t;

namespace icinga
{

struct CompilerDebugInfo
{
	const char *Path;

	int FirstLine;
	int FirstColumn;

	int LastLine;
	int LastColumn;

	operator DebugInfo(void) const
	{
		DebugInfo di;
		di.Path = Path;
		di.FirstLine = FirstLine;
		di.FirstColumn = FirstColumn;
		di.LastLine = LastLine;
		di.LastColumn = LastColumn;
		return di;
	}
};

struct EItemInfo
{
	bool SideEffect;
	CompilerDebugInfo DebugInfo;
};

struct ZoneFragment
{
	String Tag;
	String Path;
};

/**
 * The configuration compiler can be used to compile a configuration file
 * into a number of configuration items.
 *
 * @ingroup config
 */
class I2_CONFIG_API ConfigCompiler
{
public:
	explicit ConfigCompiler(const String& path, std::istream *input,
	    const String& zone = String(), const String& module = String());
	virtual ~ConfigCompiler(void);

	Expression *Compile(void);

	static Expression *CompileStream(const String& path, std::istream *stream,
	    bool async = true, const String& zone = String(), const String& module = String());
	static Expression *CompileFile(const String& path, bool async = true,
	    const String& zone = String(), const String& module = String());
	static Expression *CompileText(const String& path, const String& text,
	    bool async = true, const String& zone = String(), const String& module = String());

	static void AddIncludeSearchDir(const String& dir);

	const char *GetPath(void) const;

	void SetZone(const String& zone);
	String GetZone(void) const;
	
	void SetModule(const String& module);
	String GetModule(void) const;

	static void CollectIncludes(std::vector<Expression *>& expressions,
	    const String& file, const String& zone, const String& module);

	/* internally used methods */
	Expression *HandleInclude(const String& include, bool search, const DebugInfo& debuginfo = DebugInfo());
	Expression *HandleIncludeRecursive(const String& path, const String& pattern, const DebugInfo& debuginfo = DebugInfo());
	Expression *HandleIncludeZones(const String& tag, const String& path, const String& pattern, const DebugInfo& debuginfo = DebugInfo());
	void HandleLibrary(const String& library);

	size_t ReadInput(char *buffer, size_t max_bytes);
	void *GetScanner(void) const;

	static std::vector<ZoneFragment> GetZoneDirs(const String& zone);
	static void RegisterZoneDir(const String& tag, const String& ppath, const String& zoneName);

	static const std::vector<String>& GetKeywords(void);

private:
	boost::promise<boost::shared_ptr<Expression> > m_Promise;

	String m_Path;
	std::istream *m_Input;
	String m_Zone;
	String m_Module;

	void *m_Scanner;

	static std::vector<String> m_IncludeSearchDirs;
	static std::map<String, std::vector<ZoneFragment> > m_ZoneDirs;

	void InitializeScanner(void);
	void DestroyScanner(void);

	void CompileHelper(void);

	void HandleIncludeZone(const String& tag, const String& path, const String& pattern, std::vector<Expression *>& expressions);

public:
	bool m_Eof;
	int m_OpenBraces;

	int m_IgnoreNewlines;
	std::ostringstream m_LexBuffer;
	CompilerDebugInfo m_LocationBegin;

	std::stack<bool> m_Apply;
	std::stack<bool> m_ObjectAssign;
	std::stack<bool> m_SeenAssign;
	std::stack<bool> m_SeenIgnore;
	std::stack<Expression *> m_Assign;
	std::stack<Expression *> m_Ignore;
	std::stack<String> m_FKVar;
	std::stack<String> m_FVVar;
	std::stack<Expression *> m_FTerm;
};

}

#endif /* CONFIGCOMPILER_H */
