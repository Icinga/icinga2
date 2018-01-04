/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef FEATUREDISABLECOMMAND_H
#define FEATUREDISABLECOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "feature disable" command.
 *
 * @ingroup cli
 */
class FeatureDisableCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(FeatureDisableCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	int GetMinArguments() const override;
	int GetMaxArguments() const override;
	std::vector<String> GetPositionalSuggestions(const String& word) const override;
	ImpersonationLevel GetImpersonationLevel() const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

};

}

#endif /* FEATUREDISABLECOMMAND_H */
