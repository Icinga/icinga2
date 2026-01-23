/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/featureenablecommand.hpp"
#include "cli/featureutility.hpp"
#include "base/logger.hpp"

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/enable", FeatureEnableCommand);

String FeatureEnableCommand::GetDescription() const
{
	return "Enables specified Icinga 2 feature.";
}

String FeatureEnableCommand::GetShortDescription() const
{
	return "enables specified feature";
}

std::vector<String> FeatureEnableCommand::GetPositionalSuggestions(const String& word) const
{
	return FeatureUtility::GetFieldCompletionSuggestions(word, true);
}

int FeatureEnableCommand::GetMinArguments() const
{
	return 1;
}

int FeatureEnableCommand::GetMaxArguments() const
{
	return -1;
}

ImpersonationLevel FeatureEnableCommand::GetImpersonationLevel() const
{
	return ImpersonateIcinga;
}

/**
 * The entry point for the "feature enable" CLI command.
 *
 * @returns An exit status.
 */
int FeatureEnableCommand::Run(const boost::program_options::variables_map&, const std::vector<std::string>& ap) const
{
	return FeatureUtility::EnableFeatures(ap);
}
