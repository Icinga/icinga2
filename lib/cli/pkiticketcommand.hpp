// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PKITICKETCOMMAND_H
#define PKITICKETCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "pki ticket" command.
 *
 * @ingroup cli
 */
class PKITicketCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(PKITicketCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}

#endif /* PKITICKETCOMMAND_H */
