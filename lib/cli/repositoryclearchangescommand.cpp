/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "cli/repositoryclearchangescommand.hpp"
#include "cli/repositoryutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("repository/clear-changes", RepositoryClearChangesCommand);

String RepositoryClearChangesCommand::GetDescription(void) const
{
	return "Clear uncommitted Icinga 2 repository changes";
}

String RepositoryClearChangesCommand::GetShortDescription(void) const
{
	return "clear uncommitted repository changes";
}

ImpersonationLevel RepositoryClearChangesCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

bool RepositoryClearChangesCommand::IsDeprecated(void) const
{
	return true;
}

/**
 * The entry point for the "repository clear-changes" CLI command.
 *
 * @returns An exit status.
 */
int RepositoryClearChangesCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!Utility::PathExists(RepositoryUtility::GetRepositoryChangeLogPath())) {
		std::cout << "Repository Changelog path '" << RepositoryUtility::GetRepositoryChangeLogPath() << "' does not exist. Add objects first!\n";
		return 1;
	}

	std::cout << "Clearing all remaining changes\n";
	RepositoryUtility::ClearChangeLog();

	return 0;
}
