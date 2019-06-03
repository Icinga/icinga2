/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "cli/featurelistcommand.hpp"
#include "cli/featureutility.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/console.hpp"
#include <boost/algorithm/string/join.hpp>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/list", FeatureListCommand);

String FeatureListCommand::GetDescription() const
{
	return "Lists all available Icinga 2 features.";
}

String FeatureListCommand::GetShortDescription() const
{
	return "lists all available features";
}

/**
 * The entry point for the "feature list" CLI command.
 *
 * @returns An exit status.
 */
int FeatureListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	return FeatureUtility::ListFeatures();
}
