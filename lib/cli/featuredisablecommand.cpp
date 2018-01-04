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

#include "cli/featuredisablecommand.hpp"
#include "cli/featureutility.hpp"
#include "base/logger.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/disable", FeatureDisableCommand);

String FeatureDisableCommand::GetDescription() const
{
	return "Disables specified Icinga 2 feature.";
}

String FeatureDisableCommand::GetShortDescription() const
{
	return "disables specified feature";
}

std::vector<String> FeatureDisableCommand::GetPositionalSuggestions(const String& word) const
{
	return FeatureUtility::GetFieldCompletionSuggestions(word, false);
}

int FeatureDisableCommand::GetMinArguments() const
{
	return 1;
}

int FeatureDisableCommand::GetMaxArguments() const
{
	return -1;
}

ImpersonationLevel FeatureDisableCommand::GetImpersonationLevel() const
{
	return ImpersonateRoot;
}

/**
 * The entry point for the "feature disable" CLI command.
 *
 * @returns An exit status.
 */
int FeatureDisableCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (ap.empty()) {
		Log(LogCritical, "cli", "Cannot disable feature(s). Name(s) are missing!");
		return 0;
	}

	return FeatureUtility::DisableFeatures(ap);
}
