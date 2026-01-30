// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PKISAVECERTCOMMAND_H
#define PKISAVECERTCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "pki save-cert" command.
 *
 * @ingroup cli
 */
class PKISaveCertCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(PKISaveCertCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	std::vector<String> GetArgumentSuggestions(const String& argument, const String& word) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}

#endif /* PKISAVECERTCOMMAND_H */
