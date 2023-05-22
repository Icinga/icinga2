/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "node wizard" command.
 *
 * @ingroup cli
 */
class NodeWizardCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(NodeWizardCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int GetMaxArguments() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
	ImpersonationLevel GetImpersonationLevel() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;

private:
	int AgentSatelliteSetup() const;
	int MasterSetup() const;
};

}
