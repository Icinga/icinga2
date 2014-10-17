/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "cli/pkinewcacommand.hpp"
#include "base/logger_fwd.hpp"
#include "base/clicommand.hpp"
#include "base/application.hpp"
#include "base/tlsutility.hpp"
#include <fstream>

using namespace icinga;

REGISTER_CLICOMMAND("pki/new-ca", PKINewCACommand);

String PKINewCACommand::GetDescription(void) const
{
	return "Sets up a new Certificate Authority.";
}

String PKINewCACommand::GetShortDescription(void) const
{
	return "sets up a new CA";
}

/**
 * The entry point for the "pki new-ca" CLI command.
 *
 * @returns An exit status.
 */
int PKINewCACommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String cadir = Application::GetLocalStateDir() + "/lib/icinga2/ca";

	if (Utility::PathExists(cadir)) {
		Log(LogCritical, "base", "CA directory '" + cadir + "' already exists.");
		return 1;
	}

	if (!Utility::MkDirP(cadir, 0700)) {
		Log(LogCritical, "base", "Could not create CA directory '" + cadir + "'.");
		return 1;
	}

	MakeX509CSR("Icinga CA", cadir + "/ca.key", String(), cadir + "/ca.crt", true);

	String serialpath = cadir + "/serial.txt";

	Log(LogInformation, "cli", "Initializing serial file in '" + serialpath + "'.");

	std::ofstream fp;
	fp.open(serialpath.CStr());
	fp << "01";
	fp.close();

	return 0;
}
