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
class I2_REMOTE_API ConfigModuleUtility
{

public:
	static String GetModuleDir(void);

	static void CreateModule(const String& name);
	static void DeleteModule(const String& name);
	static std::vector<String> GetModules(void);

	static String CreateStage(const String& moduleName, const Dictionary::Ptr& files);
	static void DeleteStage(const String& moduleName, const String& stageName);
	static std::vector<String> GetStages(const String& moduleName);
	static String GetActiveStage(const String& moduleName);
	static void ActivateStage(const String& moduleName, const String& stageName);
	static void AsyncTryActivateStage(const String& moduleName, const String& stageName);

	static std::vector<std::pair<String, bool> > GetFiles(const String& moduleName, const String& stageName);

	static bool ContainsDotDot(const String& path);
	static bool ValidateName(const String& name);

private:
	static void CollectDirNames(const String& path, std::vector<String>& dirs);
	static void CollectPaths(const String& path, std::vector<std::pair<String, bool> >& paths);

	static void WriteModuleConfig(const String& moduleName);
	static void WriteStageConfig(const String& moduleName, const String& stageName);

	static void TryActivateStageCallback(const ProcessResult& pr, const String& moduleName, const String& stageName);
};

}

#endif /* CONFIGMODULEUTILITY_H */
