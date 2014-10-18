/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/featureenablecommand.hpp"
#include "cli/featureutility.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <fstream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/enable", FeatureEnableCommand);

String FeatureEnableCommand::GetDescription(void) const
{
	return "Enables specified Icinga 2 feature.";
}

String FeatureEnableCommand::GetShortDescription(void) const
{
	return "enables specified feature";
}

std::vector<String> FeatureEnableCommand::GetPositionalSuggestions(const String& word) const
{
	return FeatureUtility::GetFieldCompletionSuggestions(FeatureCommandEnable, word);
}

/**
 * The entry point for the "feature enable" CLI command.
 *
 * @returns An exit status.
 */
int FeatureEnableCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String features_available_dir = Application::GetSysconfDir() + "/icinga2/features-available";
	String features_enabled_dir = Application::GetSysconfDir() + "/icinga2/features-enabled";

	if (ap.empty()) {
		Log(LogCritical, "cli", "Cannot enable feature(s). Name(s) are missing!");
		return 0;
	}

	if (!Utility::PathExists(features_available_dir) ) {
		Log(LogCritical, "cli", "Cannot parse available features. Path '" + features_available_dir + "' does not exist.");
		return 0;
	}

	if (!Utility::PathExists(features_enabled_dir) ) {
		Log(LogCritical, "cli", "Cannot enable features. Path '" + features_enabled_dir + "' does not exist.");
		return 0;
	}

	std::vector<std::string> errors;

	BOOST_FOREACH(const String& feature, ap) {
		String source = features_available_dir + "/" + feature + ".conf";

		if (!Utility::PathExists(source) ) {
			Log(LogCritical, "cli", "Cannot enable feature '" + feature + "'. Source file '" + source + "' does not exist.");
			errors.push_back(feature);
			continue;
		}

		String target = features_enabled_dir + "/" + feature + ".conf";

		if (Utility::PathExists(target) ) {
			Log(LogWarning, "cli", "Feature '" + feature + "' already enabled.");
			continue;
		}

#ifndef _WIN32
		if (symlink(source.CStr(), target.CStr()) < 0) {
			Log(LogCritical, "cli", "Cannot enable feature '" + feature + "'. Linking source '" + source + "' to target file '" + target +
			    "' failed with error code " + Convert::ToString(errno) + ", \"" + Utility::FormatErrorNumber(errno) + "\".");
			errors.push_back(feature);
			continue;
		}
#else /* _WIN32 */
		std::ofstream fp;
		fp.open(target.CStr());
		if (!fp) {
			Log(LogCritical, "cli", "Cannot enable feature '" + feature + "'. Failed to open file '" + target + "'.");
			errors.push_back(feature);
			continue;
		}
		fp << "include \"../features-available/" << feature << ".conf\"" << std::endl;
		fp.close();
#endif /* _WIN32 */

		std::cout << "Enabling feature " << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << feature
		    << ConsoleColorTag(Console_Normal) << ". Make sure to restart Icinga 2 for these changes to take effect.\n";
	}

	if (!errors.empty()) {
		Log(LogCritical, "cli", "Cannot enable feature(s): " + boost::algorithm::join(errors, " "));
		errors.clear();
		return 1;
	}

	return 0;
}
