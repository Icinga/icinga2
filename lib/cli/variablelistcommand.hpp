// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VARIABLELISTCOMMAND_H
#define VARIABLELISTCOMMAND_H

#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "cli/clicommand.hpp"
#include <ostream>

namespace icinga
{

/**
 * The "variable list" command.
 *
 * @ingroup cli
 */
class VariableListCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(VariableListCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

private:
	static void PrintVariable(std::ostream& fp, const String& message);
};

}

#endif /* VARIABLELISTCOMMAND_H */
