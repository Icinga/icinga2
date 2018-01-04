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

	virtual String GetDescription(void) const override;
	virtual String GetShortDescription(void) const override;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
};

}

#endif /* FEATURELISTCOMMAND_H */
