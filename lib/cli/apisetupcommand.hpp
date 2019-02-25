/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APISETUPCOMMAND_H
#define APISETUPCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "api setup" command.
 *
 * @ingroup cli
 */
class ApiSetupCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(ApiSetupCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int GetMaxArguments() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
	ImpersonationLevel GetImpersonationLevel() const override;
};

}

#endif /* APISETUPCOMMAND_H */
