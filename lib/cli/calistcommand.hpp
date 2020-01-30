/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "ca list" command.
 *
 * @ingroup cli
 */
class CAListCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(CAListCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

private:
	static void PrintRequest(const String& requestFile);
};

}
