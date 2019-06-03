/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef PKINEWCACOMMAND_H
#define PKINEWCACOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "pki new-ca" command.
 *
 * @ingroup cli
 */
class PKINewCACommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(PKINewCACommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}

#endif /* PKINEWCACOMMAND_H */
