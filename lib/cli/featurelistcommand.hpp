/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "feature list" command.
 *
 * @ingroup cli
 */
class FeatureListCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(FeatureListCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}
