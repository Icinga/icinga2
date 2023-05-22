/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
