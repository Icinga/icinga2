/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#pragma once

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "pki verify" command.
 *
 * @ingroup cli
 */
class PKIVerifyCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(PKIVerifyCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	std::vector<String> GetArgumentSuggestions(const String& argument, const String& word) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}
