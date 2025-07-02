/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef FEATUREDISABLECOMMAND_H
#define FEATUREDISABLECOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "feature disable" command.
 *
 * @ingroup cli
 */
class FeatureDisableCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(FeatureDisableCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int GetMinArguments() const override;
	int GetMaxArguments() const override;
	std::vector<String> GetPositionalSuggestions(const String& word) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}

#endif /* FEATUREDISABLECOMMAND_H */
