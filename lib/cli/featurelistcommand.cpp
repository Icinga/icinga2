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

#include "cli/featurelistcommand.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <unistd.h>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/list", FeatureListCommand);

String FeatureListCommand::GetDescription(void) const
{
	return "Lists all enabled Icinga 2 features.";
}

String FeatureListCommand::GetShortDescription(void) const
{
	return "lists all enabled features";
}

void FeatureListCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc,
    ArgumentCompletionDescription& argCompletionDesc) const
{
	/* Command doesn't support any parameters. */
}

/**
 * The entry point for the "feature list" CLI command.
 *
 * @returns An exit status.
 */
int FeatureListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!ap.empty()) {
		Log(LogWarning, "cli", "Ignoring parameters: " + boost::algorithm::join(ap, " "));
	}

#ifdef _WIN32
	//TODO: Add Windows support
	Log(LogInformation, "cli", "This command is not available on Windows.");
#else
	std::vector<String> enabled_features;
	std::vector<String> available_features;

	if (!Utility::Glob(Application::GetSysconfDir() + "/icinga2/features-enabled/*.conf",
	    boost::bind(&FeatureListCommand::CollectFeatures, _1, boost::ref(enabled_features)), GlobFile)) {
		Log(LogCritical, "cli", "Cannot access path '" + Application::GetSysconfDir() + "/icinga2/features-enabled/'.");
	}

	if (!Utility::Glob(Application::GetSysconfDir() + "/icinga2/features-available/*.conf",
	    boost::bind(&FeatureListCommand::CollectFeatures, _1, boost::ref(available_features)), GlobFile)) {
		Log(LogCritical, "cli", "Cannot access path '" + Application::GetSysconfDir() + "/icinga2/available-available/'.");
	}

	Log(LogInformation, "cli", "Available features: " + boost::algorithm::join(available_features, " "));
	Log(LogInformation, "cli", "---");
	Log(LogInformation, "cli", "Enabled features: " + boost::algorithm::join(enabled_features, " "));
#endif /* _WIN32 */

	return 0;
}

void FeatureListCommand::CollectFeatures(const String& feature_file, std::vector<String>& features)
{
	String feature = Utility::BaseName(feature_file);
	boost::algorithm::replace_all(feature, ".conf", "");

	Log(LogDebug, "cli", "Adding feature: " + feature);
	features.push_back(feature);
}
