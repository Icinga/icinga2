/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

static bool ExecuteExpression(Expression *expression)
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

static void IncludeZoneDirRecursive(const String& path, const String& package, bool& success)
{
	String zoneName = Utility::BaseName(path);

	/* register this zone path for cluster config sync */
	ConfigCompiler::RegisterZoneDir("_etc", path, zoneName);

	std::vector<Expression *> expressions;
	Utility::GlobRecursive(path, "*.conf", std::bind(&ConfigCompiler::CollectIncludes, std::ref(expressions), _1, zoneName, package), GlobFile);
	DictExpression expr(expressions);
	if (!ExecuteExpression(&expr))
		success = false;
}

static void IncludeNonLocalZone(const String& zonePath, const String& package, bool& success)
{
	/* Note: This include function must not call RegisterZoneDir().
	 * We do not need to copy it for cluster config sync. */

	String zoneName = Utility::BaseName(zonePath);

	/* Check whether this node already has an authoritative config version
	 * from zones.d in etc or api package directory, or a local marker file)
	 */
	if (ConfigCompiler::HasZoneConfigAuthority(zoneName) || Utility::PathExists(zonePath + "/.authoritative")) {
		Log(LogNotice, "config")
		    << "Ignoring non local config include for zone '" << zoneName << "': We already have an authoritative copy included.";
		return;
	}

	std::vector<Expression *> expressions;
	Utility::GlobRecursive(zonePath, "*.conf", std::bind(&ConfigCompiler::CollectIncludes, std::ref(expressions), _1, zoneName, package), GlobFile);
	DictExpression expr(expressions);
	if (!ExecuteExpression(&expr))
		success = false;
}

static void IncludePackage(const String& packagePath, bool& success)
{
	/* Note: Package includes will register their zones
	 * for config sync inside their generated config. */
	String packageName = Utility::BaseName(packagePath);

	if (Utility::PathExists(packagePath + "/include.conf")) {
		Expression *expr = ConfigCompiler::CompileFile(packagePath + "/include.conf",
		    String(), packageName);

		if (!ExecuteExpression(expr))
			success = false;

		delete expr;
	}
}

bool DaemonUtility::ValidateConfigFiles(const std::vector<std::string>& configs, const String& objectsFile)
{
	bool success;
	if (!objectsFile.IsEmpty())
		ConfigCompilerContext::GetInstance()->OpenObjectsFile(objectsFile);

	if (!configs.empty()) {
		for (const String& configPath : configs) {
			try {
				Expression *expression = ConfigCompiler::CompileFile(configPath, String(), "_etc");
				success = ExecuteExpression(expression);
				delete expression;
				if (!success)
					return false;
			} catch (const std::exception& ex) {
				Log(LogCritical, "cli", "Could not compile config files: " + DiagnosticInformation(ex, false));
				Application::Exit(1);
			}
		}
	}

	/* Load cluster config files from /etc/icinga2/zones.d.
	 * This should probably be in libremote but
	 * unfortunately moving it there is somewhat non-trivial. */
	success = true;

	String zonesEtcDir = Application::GetZonesDir();
	if (!zonesEtcDir.IsEmpty() && Utility::PathExists(zonesEtcDir))
		Utility::Glob(zonesEtcDir + "/*", std::bind(&IncludeZoneDirRecursive, _1, "_etc", std::ref(success)), GlobDirectory);

	if (!success)
		return false;

	/* Load package config files - they may contain additional zones which
	 * are authoritative on this node and are checked in HasZoneConfigAuthority(). */
	String packagesVarDir = Application::GetLocalStateDir() + "/lib/icinga2/api/packages";
	if (Utility::PathExists(packagesVarDir))
		Utility::Glob(packagesVarDir + "/*", std::bind(&IncludePackage, _1, std::ref(success)), GlobDirectory);

	if (!success)
		return false;

	/* Load cluster synchronized configuration files */
	String zonesVarDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones";
	if (Utility::PathExists(zonesVarDir))
		Utility::Glob(zonesVarDir + "/*", std::bind(&IncludeNonLocalZone, _1, "_cluster", std::ref(success)), GlobDirectory);

	if (!success)
		return false;

	Type::Ptr appType = Type::GetByName(ScriptGlobal::Get("ApplicationType", &Empty));

	if (ConfigItem::GetItems(appType).empty()) {
		ConfigItemBuilder::Ptr builder = new ConfigItemBuilder();
		builder->SetType(appType);
		builder->SetName("app");
		builder->AddExpression(new ImportDefaultTemplatesExpression());
		ConfigItem::Ptr item = builder->Compile();
		item->Register();
	}

	return true;
}

bool DaemonUtility::LoadConfigFiles(const std::vector<std::string>& configs,
    std::vector<ConfigItem::Ptr>& newItems,
    const String& objectsFile, const String& varsfile)
{
	ActivationScope ascope;

	if (!DaemonUtility::ValidateConfigFiles(configs, objectsFile)) {
		ConfigCompilerContext::GetInstance()->CancelObjectsFile();
		return false;
	}

	WorkQueue upq(25000, Application::GetConcurrency());
	upq.SetName("DaemonUtility::LoadConfigFiles");
	bool result = ConfigItem::CommitItems(ascope.GetContext(), upq, newItems);

	if (!result) {
		ConfigCompilerContext::GetInstance()->CancelObjectsFile();
		return false;
	}

	ConfigCompilerContext::GetInstance()->FinishObjectsFile();

	try {
		ScriptGlobal::WriteToFile(varsfile);
	} catch (const std::exception& ex) {
		Log(LogCritical, "cli", "Could not write vars file: " + DiagnosticInformation(ex, false));
		Application::Exit(1);
	}

	return true;
}
