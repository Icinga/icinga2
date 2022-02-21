/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGCOMPILER_H
#define CONFIGCOMPILER_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"
#include "base/registry.hpp"
#include "base/initialize.hpp"
#include "base/singleton.hpp"
#include "base/string.hpp"
#include <future>
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

	operator DebugInfo() const
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

enum FlowControlType
{
	FlowControlReturn = 1,
	FlowControlContinue = 2,
	FlowControlBreak = 4
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
class ConfigCompiler
{
public:
	explicit ConfigCompiler(String path, std::istream *input,
		String zone = String(), String package = String());
	virtual ~ConfigCompiler();

	std::unique_ptr<Expression> Compile();

	static std::unique_ptr<Expression>CompileStream(const String& path, std::istream *stream,
		const String& zone = String(), const String& package = String());
	static std::unique_ptr<Expression>CompileFile(const String& path, const String& zone = String(),
		const String& package = String());
	static std::unique_ptr<Expression>CompileText(const String& path, const String& text,
		const String& zone = String(), const String& package = String());

	static void AddIncludeSearchDir(const String& dir);

	const char *GetPath() const;

	void SetZone(const String& zone);
	String GetZone() const;

	void SetPackage(const String& package);
	String GetPackage() const;

	void AddImport(const Expression::Ptr& import);
	std::vector<Expression::Ptr> GetImports() const;

	static void CollectIncludes(std::vector<std::unique_ptr<Expression> >& expressions,
		const String& file, const String& zone, const String& package);

	static std::unique_ptr<Expression> HandleInclude(const String& relativeBase, const String& path, bool search,
		const String& zone, const String& package, const DebugInfo& debuginfo = DebugInfo());
	static std::unique_ptr<Expression> HandleIncludeRecursive(const String& relativeBase, const String& path,
		const String& pattern, const String& zone, const String& package, const DebugInfo& debuginfo = DebugInfo());
	static std::unique_ptr<Expression> HandleIncludeZones(const String& relativeBase, const String& tag,
		const String& path, const String& pattern, const String& package, const DebugInfo& debuginfo = DebugInfo());

	size_t ReadInput(char *buffer, size_t max_bytes);
	void *GetScanner() const;

	static std::vector<ZoneFragment> GetZoneDirs(const String& zone);
	static void RegisterZoneDir(const String& tag, const String& ppath, const String& zoneName);

	static bool HasZoneConfigAuthority(const String& zoneName);

private:
	std::promise<Expression::Ptr> m_Promise;

	String m_Path;
	std::istream *m_Input;
	String m_Zone;
	String m_Package;
	std::vector<Expression::Ptr> m_Imports;

	void *m_Scanner;

	static std::vector<String> m_IncludeSearchDirs;
	static std::mutex m_ZoneDirsMutex;
	static std::map<String, std::vector<ZoneFragment> > m_ZoneDirs;

	void InitializeScanner();
	void DestroyScanner();

	static void HandleIncludeZone(const String& relativeBase, const String& tag, const String& path, const String& pattern, const String& package, std::vector<std::unique_ptr<Expression> >& expressions);

	static bool IsAbsolutePath(const String& path);

public:
	bool m_Eof;
	int m_OpenBraces;

	String m_LexBuffer;
	CompilerDebugInfo m_LocationBegin;

	std::stack<bool> m_IgnoreNewlines;
	std::stack<bool> m_Apply;
	std::stack<bool> m_ObjectAssign;
	std::stack<bool> m_SeenAssign;
	std::stack<bool> m_SeenIgnore;
	std::stack<Expression *> m_Assign;
	std::stack<Expression *> m_Ignore;
	std::stack<String> m_FKVar;
	std::stack<String> m_FVVar;
	std::stack<Expression *> m_FTerm;
	std::stack<int> m_FlowControlInfo;
};

}

#endif /* CONFIGCOMPILER_H */
