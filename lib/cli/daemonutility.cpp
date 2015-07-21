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

#include "cli/daemonutility.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configitembuilder.hpp"


using namespace icinga;

bool ExecuteExpression(Expression *expression)
{
	if (!expression)
		return false;

	try {
		ScriptFrame frame;
		expression->Evaluate(frame);
	} catch (const std::exception& ex) {
		Log(LogCritical, "config", DiagnosticInformation(ex));
		return false;
	}

	return true;
}

void IncludeZoneDirRecursive(const String& path, bool& success)
{
	String zoneName = Utility::BaseName(path);

	std::vector<Expression *> expressions;
	Utility::GlobRecursive(path, "*.conf", boost::bind(&ConfigCompiler::CollectIncludes, boost::ref(expressions), _1, zoneName), GlobFile);
	DictExpression expr(expressions);
	if (!ExecuteExpression(&expr))
		success = false;
}

void IncludeNonLocalZone(const String& zonePath, bool& success)
{
	String etcPath = Application::GetZonesDir() + "/" + Utility::BaseName(zonePath);

	if (Utility::PathExists(etcPath) || Utility::PathExists(zonePath + "/.authoritative"))
		return;

	IncludeZoneDirRecursive(zonePath, success);
}

bool DaemonUtility::ValidateConfigFiles(const std::vector<std::string>& configs, const String& objectsFile)
{
	bool success;
	if (!objectsFile.IsEmpty())
		ConfigCompilerContext::GetInstance()->OpenObjectsFile(objectsFile);

	if (!configs.empty()) {
		BOOST_FOREACH(const String& configPath, configs) {
			Expression *expression = ConfigCompiler::CompileFile(configPath);
			success = ExecuteExpression(expression);
			delete expression;
			if (!success)
				return false;
		}
	}

	/* Load cluster config files - this should probably be in libremote but
	* unfortunately moving it there is somewhat non-trivial. */
	success = true;

	String zonesEtcDir = Application::GetZonesDir();
	if (!zonesEtcDir.IsEmpty() && Utility::PathExists(zonesEtcDir))
		Utility::Glob(zonesEtcDir + "/*", boost::bind(&IncludeZoneDirRecursive, _1, boost::ref(success)), GlobDirectory);

	if (!success)
		return false;

	String zonesVarDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones";
	if (Utility::PathExists(zonesVarDir))
		Utility::Glob(zonesVarDir + "/*", boost::bind(&IncludeNonLocalZone, _1, boost::ref(success)), GlobDirectory);

	if (!success)
		return false;

	String modulesVarDir = Application::GetLocalStateDir() + "/lib/icinga2/api/modules";
	if (Utility::PathExists(modulesVarDir)) {
		std::vector<Expression *> expressions;
		Utility::Glob(modulesVarDir + "/*/include.conf", boost::bind(&ConfigCompiler::CollectIncludes, boost::ref(expressions), _1, ""), GlobFile);
		DictExpression expr(expressions);
		if (!ExecuteExpression(&expr))
			success = false;
	}

	if (!success)
		return false;

	return true;
}

bool DaemonUtility::LoadConfigFiles(const std::vector<std::string>& configs,
    const String& objectsFile, const String& varsfile)
{
	if (!DaemonUtility::ValidateConfigFiles(configs, objectsFile))
		return false;

	bool result = ConfigItem::CommitItems();

	if (!result)
		return false;

	ConfigCompilerContext::GetInstance()->FinishObjectsFile();
	ScriptGlobal::WriteToFile(varsfile);

	return true;
}
