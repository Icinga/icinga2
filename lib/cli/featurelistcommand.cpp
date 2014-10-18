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
#include "cli/featureutility.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/convert.hpp"
#include "base/console.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <iostream>

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

	std::vector<String> available_features;
	std::vector<String> disabled_features;
	std::vector<String> enabled_features;

	if (!FeatureUtility::GetFeatures(FeaturesAvailable, available_features))
		return 1;
	if (!FeatureUtility::GetFeatures(FeaturesDisabled, disabled_features))
		return 1;
	if (!FeatureUtility::GetFeatures(FeaturesEnabled, enabled_features))
		return 1;

	std::cout << ConsoleColorTag(Console_ForegroundBlue | Console_Bold) << "Available features: " << ConsoleColorTag(Console_Normal)
	    << boost::algorithm::join(available_features, " ") << "\n";
	std::cout << ConsoleColorTag(Console_ForegroundRed | Console_Bold) << "Disabled features: " << ConsoleColorTag(Console_Normal)
	    << boost::algorithm::join(disabled_features, " ") << "\n";
	std::cout << ConsoleColorTag(Console_ForegroundGreen | Console_Bold) << "Enabled features: " << ConsoleColorTag(Console_Normal)
	    << boost::algorithm::join(enabled_features, " ") << "\n";

	return 0;
}