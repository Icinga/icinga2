/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef DAEMONCOMMAND_H
#define DAEMONCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "daemon" CLI command.
 *
 * @ingroup cli
 */
class DaemonCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(DaemonCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	std::vector<String> GetArgumentSuggestions(const String& argument, const String& word) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}

#endif /* DAEMONCOMMAND_H */
