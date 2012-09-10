/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include <i2-icinga.h>

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION VERSION ", " GIT_MESSAGE

#	include <ltdl.h>
#else /* _WIN32 */
#	define ICINGA_VERSION VERSION
#endif /* _WIN32 */

using namespace icinga;

/**
 * Entry point for the Icinga application.
 *
 * @params argc Number of command line arguments.
 * @params argv Command line arguments.
 * @returns The application's exit status.
 */
int main(int argc, char **argv)
{
#ifndef _WIN32
	LTDL_SET_PRELOADED_SYMBOLS();
#endif /* _WIN32 */

#ifndef _WIN32
	lt_dlinit();
#endif /* _WIN32 */

	/* This must be done before calling any other functions
	 * in the base library. */
	Application::SetMainThread();

	Logger::Write(LogInformation, "icinga", "Icinga application loader (version: " ICINGA_VERSION ")");

	if (argc < 3 || strcmp(argv[1], "-c") != 0) {
		stringstream msgbuf;
		msgbuf << "Syntax: " << argv[0] << " -c <config-file> ...";    
		Logger::Write(LogInformation, "base", msgbuf.str());
		return EXIT_FAILURE;
	}

	String componentDirectory = Utility::DirName(Application::GetExePath(argv[0])) + "/../lib/icinga2";
	Component::AddSearchDir(componentDirectory);

	DynamicObject::BeginTx();

	/* load config file */
	String configFile = argv[2];
	vector<ConfigItem::Ptr> configItems = ConfigCompiler::CompileFile(configFile);

	Logger::Write(LogInformation, "icinga", "Executing config items...");  

	BOOST_FOREACH(const ConfigItem::Ptr& item, configItems) {
		item->Commit();
	}

	DynamicObject::FinishTx();

	Application::Ptr app = Application::GetInstance();

	if (!app)
		throw_exception(runtime_error("Configuration must create an Application object."));

	/* The application class doesn't need to know about the "-c configFile"
	 * command-line arguments. */
	return app->Run(argc - 2, &(argv[2]));
}

