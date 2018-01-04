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

#include "cli/featurelistcommand.hpp"
#include "cli/featureutility.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/console.hpp"
#include <boost/algorithm/string/join.hpp>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("feature/list", FeatureListCommand);

String FeatureListCommand::GetDescription() const
{
	return "Lists all available Icinga 2 features.";
}

String FeatureListCommand::GetShortDescription() const
{
	return "lists all available features";
}

/**
 * The entry point for the "feature list" CLI command.
 *
 * @returns An exit status.
 */
int FeatureListCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	return FeatureUtility::ListFeatures();
}
