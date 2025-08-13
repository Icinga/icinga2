/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
