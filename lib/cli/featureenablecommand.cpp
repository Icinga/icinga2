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
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/convert.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>

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

void FeatureEnableCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc,
    ArgumentCompletionDescription& argCompletionDesc) const
{
        /* Command doesn't support any parameters. */
}

/**
 * The entry point for the "feature enable" CLI command.
 *
 * @returns An exit status.
 */
int FeatureEnableCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
#ifdef _WIN32
	//TODO: Add Windows support
        Log(LogInformation, "cli", "This command is not available on Windows.");
#else
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

                if (symlink(source.CStr(), target.CStr()) < 0) {
			Log(LogCritical, "cli", "Cannot enable feature '" + feature + "'. Linking source '" + source + "' to target file '" + target +
			    "' failed with error code " + Convert::ToString(errno) + ", \"" + Utility::FormatErrorNumber(errno) + "\".");
                        errors.push_back(feature);
                        continue;
		}

                Log(LogInformation, "cli", "Enabling feature '" + feature + "' in '" + features_enabled_dir + "'.");
        }

        if (!errors.empty()) {
                Log(LogCritical, "cli", "Cannot enable feature(s): " + boost::algorithm::join(errors, " "));
		errors.clear();
                return 1;
        }

#endif /* _WIN32 */

	return 0;
}
