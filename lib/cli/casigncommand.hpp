/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CASIGNCOMMAND_H
#define CASIGNCOMMAND_H

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
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}

#endif /* CASIGNCOMMAND_H */
