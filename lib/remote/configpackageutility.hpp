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
	static String GetActiveStage(const String& packageName);
	static void ActivateStage(const String& packageName, const String& stageName);
	static void AsyncTryActivateStage(const String& packageName, const String& stageName, bool reload);

	static std::vector<std::pair<String, bool> > GetFiles(const String& packageName, const String& stageName);

	static bool ContainsDotDot(const String& path);
	static bool ValidateName(const String& name);

	static boost::mutex& GetStaticMutex();

private:
	static void CollectDirNames(const String& path, std::vector<String>& dirs);
	static void CollectPaths(const String& path, std::vector<std::pair<String, bool> >& paths);

	static void WritePackageConfig(const String& packageName);
	static void WriteStageConfig(const String& packageName, const String& stageName);

	static void TryActivateStageCallback(const ProcessResult& pr, const String& packageName, const String& stageName, bool reload);
};

}

#endif /* CONFIGMODULEUTILITY_H */
