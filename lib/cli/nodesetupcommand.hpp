// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NODESETUPCOMMAND_H
#define NODESETUPCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "node setup" command.
 *
 * @ingroup cli
 */
class NodeSetupCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(NodeSetupCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	std::vector<String> GetArgumentSuggestions(const String& argument, const String& word) const override;
	ImpersonationLevel GetImpersonationLevel() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

private:
	static int SetupMaster(const boost::program_options::variables_map& vm);
	static int SetupNode(const boost::program_options::variables_map& vm);
};

}

#endif /* NODESETUPCOMMAND_H */
