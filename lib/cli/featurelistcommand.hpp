// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef FEATURELISTCOMMAND_H
#define FEATURELISTCOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "feature list" command.
 *
 * @ingroup cli
 */
class FeatureListCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(FeatureListCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}

#endif /* FEATURELISTCOMMAND_H */
