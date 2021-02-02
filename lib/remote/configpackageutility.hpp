/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONFIGMODULEUTILITY_H
#define CONFIGMODULEUTILITY_H

#include "remote/i2-remote.hpp"
#include "base/application.hpp"
#include "base/dictionary.hpp"
#include "base/process.hpp"
#include "base/string.hpp"
#include <vector>

namespace icinga
{

/**
 * Helper functions.
 *
 * @ingroup remote
 */
class ConfigPackageUtility
{

public:
	static String GetPackageDir();

	static void CreatePackage(const String& name);
	static void DeletePackage(const String& name);
	static std::vector<String> GetPackages();
	static bool PackageExists(const String& name);

	static String CreateStage(const String& packageName, const Dictionary::Ptr& files = nullptr);
	static void DeleteStage(const String& packageName, const String& stageName);
	static std::vector<String> GetStages(const String& packageName);
	static String GetActiveStageFromFile(const String& packageName);
	static String GetActiveStage(const String& packageName);
	static void SetActiveStage(const String& packageName, const String& stageName);
	static void SetActiveStageToFile(const String& packageName, const String& stageName);
	static void ActivateStage(const String& packageName, const String& stageName);
	static void AsyncTryActivateStage(const String& packageName, const String& stageName, bool activate, bool reload);

	static std::vector<std::pair<String, bool> > GetFiles(const String& packageName, const String& stageName);

	static bool ContainsDotDot(const String& path);
	static bool ValidateName(const String& name);

	static std::mutex& GetStaticPackageMutex();
	static std::mutex& GetStaticActiveStageMutex();

private:
	static void CollectDirNames(const String& path, std::vector<String>& dirs);
	static void CollectPaths(const String& path, std::vector<std::pair<String, bool> >& paths);

	static void WritePackageConfig(const String& packageName);
	static void WriteStageConfig(const String& packageName, const String& stageName);

	static void TryActivateStageCallback(const ProcessResult& pr, const String& packageName, const String& stageName, bool activate, bool reload);
};

}

#endif /* CONFIGMODULEUTILITY_H */
