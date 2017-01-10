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

#include "cli/repositorycommitcommand.hpp"
#include "cli/repositoryutility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include <fstream>
#include <iostream>

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("repository/commit", RepositoryCommitCommand);

String RepositoryCommitCommand::GetDescription(void) const
{
	return "Commit Icinga 2 repository changes";
}

String RepositoryCommitCommand::GetShortDescription(void) const
{
	return "commit repository changes";
}

void RepositoryCommitCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("simulate", "Simulate to-be-committed changes");
}

ImpersonationLevel RepositoryCommitCommand::GetImpersonationLevel(void) const
{
	return ImpersonateRoot;
}

bool RepositoryCommitCommand::IsDeprecated(void) const
{
	return true;
}

/**
 * The entry point for the "repository commit" CLI command.
 *
 * @returns An exit status.
 */
int RepositoryCommitCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	if (!Utility::PathExists(RepositoryUtility::GetRepositoryChangeLogPath())) {
		std::cout << "Repository Changelog path '" << RepositoryUtility::GetRepositoryChangeLogPath() << "' does not exist. Add objects first!\n";
		return 1;
	}

	if (vm.count("simulate")) {
		RepositoryUtility::PrintChangeLog(std::cout);
		std::cout << "\n";
		std::cout << "Simulation not yet implemented.\n";
		//TODO
		return 1;
	} else {
		RepositoryUtility::PrintChangeLog(std::cout);
		std::cout << "\n";
		RepositoryUtility::CommitChangeLog();
	}

	return 0;
}
