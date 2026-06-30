// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VARIABLEGETCOMMAND_H
#define VARIABLEGETCOMMAND_H

#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "cli/clicommand.hpp"
#include <ostream>

namespace icinga
{

/**
 * The "variable get" command.
 *
 * @ingroup cli
 */
class VariableGetCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(VariableGetCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int GetMinArguments() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}

#endif /* VARIABLEGETCOMMAND_H */
