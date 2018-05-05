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

#include "cli/apiusercommand.hpp"
#include "base/logger.hpp"
#include "base/tlsutility.hpp"
#include "base/configwriter.hpp"
#include "base/console.hpp"
#include "remote/apiuser.hpp"
#include <iostream>

#ifndef _WIN32
extern "C" {
#include <termios.h>
#include <unistd.h>
}

static void setTermEcho(bool echo) {
	termios t;
	tcgetattr(STDIN_FILENO, &t);
	if (echo) {
		t.c_lflag |= ECHO;
	} else {
		t.c_lflag &= ~ECHO;
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &t);;
}

static String promptPassword(String prompt) {
	setTermEcho(false);
	std::string password;
	std::cerr << prompt << std::flush;
	std::getline(std::cin, password);
	// manually print a new line as echo is turned off, so pressing enter
	// in the password prompt won't produce a new line in the terminal
	std::cerr << std::endl;
	setTermEcho(true);
	return password;
}
#endif

using namespace icinga;
namespace po = boost::program_options;

REGISTER_CLICOMMAND("api/user", ApiUserCommand);

String ApiUserCommand::GetDescription(void) const
{
	return "Create a hashed user and password string for the Icinga 2 API";
}

String ApiUserCommand::GetShortDescription(void) const
{
	return "API user creation helper";
}

void ApiUserCommand::InitParameters(boost::program_options::options_description& visibleDesc,
    boost::program_options::options_description& hiddenDesc) const
{
	visibleDesc.add_options()
		("user", po::value<std::string>(), "API username")
		("password", po::value<std::string>(), "Password in clear text")
		("random-password", "Generate a random password")
		("salt", po::value<std::string>(), "Optional salt (default: 8 random chars)")
		("oneline", "Print only the password hash");
}

/**
 * The entry point for the "api user" CLI command.
 *
 * @returns An exit status.
 */
int ApiUserCommand::Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const
{
	String passwd, salt;
	if (!vm.count("user") && !vm.count("oneline")) {
		Log(LogCritical, "cli", "Username (--user) must be specified.");
		return 1;
	}

	if (vm.count("password") && vm.count("random-password")) {
		Log(LogCritical, "cli", "Only one of --password and --random-password may be specified.");
		return 1;
	}

	salt = vm.count("salt") ? String(vm["salt"].as<std::string>()) : RandomString(8);
	if (salt.FindFirstOf('$') != String::NPos) {
		Log(LogCritical, "cli", "Salt (--salt) may not contain '$'");
		return 1;
	}

	if (vm.count("password")) {
		// password is passed as a command line argument
		passwd = vm["password"].as<std::string>();
	} else if (vm.count("random-password")) {
		// generating a random password was requested
		passwd = RandomString(16);
		std::cerr << "// Generated random password: " << passwd << std::endl;
#ifndef _WIN32
	} else if (isatty(STDIN_FILENO) && isatty(STDERR_FILENO)) {
		// interactive password entry
		while (true) {
			passwd = promptPassword("Enter password: ");
			if (passwd.IsEmpty()) {
				std::cerr << "No password entered, please try again." << std::endl;
				continue;
			}
			String confirm = promptPassword("Confirm password: ");
			if (passwd != confirm) {
				std::cerr << "Passwords did not match, please try again." << std::endl;
				continue;
			}
			// entered passwords are fine
			break;
		}
		std::cerr << "// Entered password: " << passwd << std::endl; // TODO: left for testing, remove later
#endif
	} else {
		std::string std_passwd;
		std::getline(std::cin, std_passwd);
		passwd = std_passwd;
	}

	String hashedPassword = CreateHashedPasswordString(passwd, salt, 5);
	if (hashedPassword == String()) {
		Log(LogCritical, "cli") << "Failed to hash password \"" << passwd << "\" with salt \"" << salt << "\"";
		return 1;
	}

	if (vm.count("oneline"))
		std::cout << hashedPassword << std::endl;
	else {
		std::cout << "object ApiUser ";

		ConfigWriter::EmitString(std::cout, vm["user"].as<std::string>());

		std::cout << "{\n"
			<< "  password_hash = ";

		ConfigWriter::EmitString(std::cout, hashedPassword);

		std::cout << "\n"
			<< "  // client_cn = \"\"\n"
			<< "\n"
			<< "  permissions = [ \"*\" ]\n"
			<< "}\n";
	}

	return 0;
}
