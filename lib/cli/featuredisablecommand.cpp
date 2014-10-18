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

#include "cli/featuredisablecommand.hpp"
#include "cli/featureutility.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <iostream>
#include <fstream>
#include <vector>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/disable", FeatureDisableCommand);

String FeatureDisableCommand::GetDescription(void) const
{
	return "Disables specified Icinga 2 feature.";
}

String FeatureDisableCommand::GetShortDescription(void) const
{
	return "disables specified feature";
}

std::vector<String> FeatureDisableCommand::GetPositionalSuggestions(const String& word) const
{
	return FeatureUtility::GetFieldCompletionSuggestions(FeatureCommandDisable, word);
}

/**
 * The entry point for the "feature disable" CLI command.
 *
 * @returns An exit status.
 */
int FeatureDisableCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
#ifdef _WIN32
	//TODO: Add Windows support
	Log(LogInformation, "cli", "This command is not available on Windows.");
#else
	String features_enabled_dir = Application::GetSysconfDir() + "/icinga2/features-enabled";

	if (ap.empty()) {
		Log(LogCritical, "cli", "Cannot disable feature(s). Name(s) are missing!");
		return 0;
	}

	if (!Utility::PathExists(features_enabled_dir) ) {
		Log(LogCritical, "cli", "Cannot disable features. Path '" + features_enabled_dir + "' does not exist.");
		return 0;
	}

	std::vector<std::string> errors;

	BOOST_FOREACH(const String& feature, ap) {
		String target = features_enabled_dir + "/" + feature + ".conf";

		if (!Utility::PathExists(target) ) {
			Log(LogCritical, "cli", "Cannot disable feature '" + feature + "'. Target file '" + target + "' does not exist.");
			errors.push_back(feature);
			continue;
		}

		if (unlink(target.CStr()) < 0) {
			Log(LogCritical, "cli", "Cannot disable feature '" + feature + "'. Unlinking target file '" + target +
			    "' failed with error code " + Convert::ToString(errno) + ", \"" + Utility::FormatErrorNumber(errno) + "\".");
			errors.push_back(feature);
			continue;
		}

		std::cout << "Disabling feature " << ConsoleColorTag(Console_ForegroundMagenta | Console_Bold) << feature
		    << ConsoleColorTag(Console_Normal) << ". Make sure to restart Icinga 2 for these changes to take effect.\n";
	}

	if (!errors.empty()) {
		Log(LogCritical, "cli", "Cannot disable feature(s): " + boost::algorithm::join(errors, " "));
		errors.clear();
		return 1;
	}

#endif /* _WIN32 */

	return 0;
}
