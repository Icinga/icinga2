/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/daemonutility.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/scriptglobal.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "config/configitembuilder.hpp"

using namespace icinga;

static bool ExecuteExpression(Expression *expression)
{
	if (!expression)
		return false;

	try {
		ScriptFrame frame(true);
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

	/* We don't have an activated zone object yet. We may forcefully guess from configitems
	 * to not include this specific synced zones directory.
	 */
	if(!ConfigItem::GetByTypeAndName(Type::GetByName("Zone"), zoneName)) {
		Log(LogWarning, "config")
			<< "Ignoring directory '" << path << "' for unknown zone '" << zoneName << "'.";
		return;
	}

	/* register this zone path for cluster config sync */
	ConfigCompiler::RegisterZoneDir("_etc", path, zoneName);

	std::vector<std::unique_ptr<Expression> > expressions;
	auto lambdaCollectIncludes = [&, zoneName, package](const String& file){
		return ConfigCompiler::CollectIncludes(expressions, file, zoneName, package);
	};
	Utility::GlobRecursive(path, "*.conf", lambdaCollectIncludes, GlobFile);
	DictExpression expr(std::move(expressions));
	if (!ExecuteExpression(&expr))
		success = false;
}

static void IncludeNonLocalZone(const String& zonePath, const String& package, bool& success)
{
	/* Note: This include function must not call RegisterZoneDir().
	 * We do not need to copy it for cluster config sync. */

	String zoneName = Utility::BaseName(zonePath);

	/* We don't have an activated zone object yet. We may forcefully guess from configitems
	 * to not include this specific synced zones directory.
	 */
	if(!ConfigItem::GetByTypeAndName(Type::GetByName("Zone"), zoneName)) {
		Log(LogWarning, "config")
			<< "Ignoring directory '" << zonePath << "' for unknown zone '" << zoneName << "'.";
		return;
	}

	/* Check whether this node already has an authoritative config version
	 * from zones.d in etc or api package directory, or a local marker file)
	 */
	if (ConfigCompiler::HasZoneConfigAuthority(zoneName) || Utility::PathExists(zonePath + "/.authoritative")) {
		Log(LogNotice, "config")
			<< "Ignoring non local config include for zone '" << zoneName << "': We already have an authoritative copy included.";
		return;
	}

	std::vector<std::unique_ptr<Expression> > expressions;
	auto lambdaCollectIncludes = [&, zoneName, package](const String& file){
		return ConfigCompiler::CollectIncludes(expressions, file, zoneName, package);
	};
	Utility::GlobRecursive(zonePath, "*.conf", lambdaCollectIncludes, GlobFile);
	DictExpression expr(std::move(expressions));
	if (!ExecuteExpression(&expr))
		success = false;
}

static void IncludePackage(const String& packagePath, bool& success)
{
	/* Note: Package includes will register their zones
	 * for config sync inside their generated config. */
	String packageName = Utility::BaseName(packagePath);

	if (Utility::PathExists(packagePath + "/include.conf")) {
		std::unique_ptr<Expression> expr = ConfigCompiler::CompileFile(packagePath + "/include.conf",
			String(), packageName);

		if (!ExecuteExpression(&*expr))
			success = false;
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
				std::unique_ptr<Expression> expression = ConfigCompiler::CompileFile(configPath, String(), "_etc");
				success = ExecuteExpression(&*expression);
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

	String zonesEtcDir = Configuration::ZonesDir;
	if (!zonesEtcDir.IsEmpty() && Utility::PathExists(zonesEtcDir)) {
		auto lambdaIncludeZoneDirRecursive = [&](const String& path){return IncludeZoneDirRecursive(path, "_etc", success);};
		Utility::Glob(zonesEtcDir + "/*", lambdaIncludeZoneDirRecursive, GlobDirectory);
	}

	if (!success)
		return false;

	/* Load package config files - they may contain additional zones which
	 * are authoritative on this node and are checked in HasZoneConfigAuthority(). */
	String packagesVarDir = Configuration::DataDir + "/api/packages";
	if (Utility::PathExists(packagesVarDir)) {
		auto lambdaIncludePackage = [&](const String &packagePath){return IncludePackage(packagePath, success);};
		Utility::Glob(packagesVarDir + "/*", lambdaIncludePackage, GlobDirectory);
	}

	if (!success)
		return false;

	/* Load cluster synchronized configuration files */
	String zonesVarDir = Configuration::DataDir + "/api/zones";
	if (Utility::PathExists(zonesVarDir)) {
		auto lambdaIncludeNonLocalZone = [&](const String& zonePath){return IncludeNonLocalZone(zonePath, "_cluster", success);};
		Utility::Glob(zonesVarDir + "/*", lambdaIncludeNonLocalZone, GlobDirectory);
	}

	if (!success)
		return false;

	Namespace::Ptr systemNS = ScriptGlobal::Get("System");
	VERIFY(systemNS);

	/* This is initialized inside the IcingaApplication class. */
	Value vAppType;
	VERIFY(systemNS->Get("ApplicationType", &vAppType));

	Type::Ptr appType = Type::GetByName(vAppType);

	if (ConfigItem::GetItems(appType).empty()) {
		ConfigItemBuilder builder;
		builder.SetType(appType);
		builder.SetName("app");
		builder.AddExpression(new ImportDefaultTemplatesExpression());
		ConfigItem::Ptr item = builder.Compile();
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

	WorkQueue upq(25000, Configuration::Concurrency);
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
