/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "ca sign" command.
 *
 * @ingroup cli
 */
class CASignCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(CASignCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int GetMinArguments() const override;
	ImpersonationLevel GetImpersonationLevel() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}
