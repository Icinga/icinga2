// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INTERNALSIGNALCOMMAND_H
#define INTERNALSIGNALCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "internal signal" command.
 *
 * @ingroup cli
 */
class InternalSignalCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(InternalSignalCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	ImpersonationLevel GetImpersonationLevel() const override;
	bool IsHidden() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}

#endif /* INTERNALSIGNALCOMMAND_H */
