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

#ifndef FEATUREENABLECOMMAND_H
#define FEATUREENABLECOMMAND_H

#include "cli/clicommand.hpp"

namespace icinga
{

/**
 * The "feature enable" command.
 *
 * @ingroup cli
 */
class FeatureEnableCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(FeatureEnableCommand);

	virtual String GetDescription() const override;
	virtual String GetShortDescription() const override;
	virtual int GetMinArguments() const override;
	virtual int GetMaxArguments() const override;
	virtual std::vector<String> GetPositionalSuggestions(const String& word) const override;
	virtual ImpersonationLevel GetImpersonationLevel() const override;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}

#endif /* FEATUREENABLECOMMAND_H */
