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
#endif /* _WIN32 */

using namespace icinga;
namespace po = boost::program_options;

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

	/* Install exception handlers to make debugging easier. */
	Application::InstallExceptionHandlers();

#ifdef ICINGA_PREFIX
	Application::SetPrefixDir(ICINGA_PREFIX);
#endif /* ICINGA_PREFIX */

#ifdef ICINGA_LOCALSTATEDIR
	Application::SetLocalStateDir(ICINGA_LOCALSTATEDIR);
#endif /* ICINGA_LOCALSTATEDIR */

#ifdef ICINGA_PKGLIBDIR
	Application::SetPkgLibDir(ICINGA_PKGLIBDIR);
#endif /* ICINGA_PKGLIBDIR */

#ifdef ICINGA_PKGDATADIR
	Application::SetPkgDataDir(ICINGA_PKGDATADIR);
#endif /* ICINGA_PKGDATADIR */

	Logger::Write(LogInformation, "icinga-app", "Icinga application loader"
#ifndef _WIN32
		" (version: " ICINGA_VERSION ")"
#endif /* _WIN32 */
	);

	po::options_description desc("Supported options");
	desc.add_options()
		("help,h", "show this help message")
		("library,l", po::value<vector<String> >(), "load a library")
		("include,I", po::value<vector<String> >(), "add include search directory")
		("config,c", po::value<vector<String> >(), "parse a configuration file")
		("validate,v", "exit after validating the configuration")
		("debug", "enable debugging")
		("daemonize,d", "daemonize after reading the configuration files")
	;

	po::variables_map vm;

	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
	} catch (const po::unknown_option& ex) {
		stringstream msgbuf;
		msgbuf << "Error while parsing command-line options: " << ex.what();
		Logger::Write(LogCritical, "icinga-app", msgbuf.str());
		return EXIT_FAILURE;
	}

	po::notify(vm);

	if (vm.count("debug"))
		Application::SetDebugging(true);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return EXIT_SUCCESS;
	}

	Component::AddSearchDir(Application::GetPkgLibDir());

	Utility::LoadIcingaLibrary("icinga", false);

	if (vm.count("library")) {
		BOOST_FOREACH(const String& libraryName, vm["library"].as<vector<String> >()) {
			Utility::LoadIcingaLibrary(libraryName, false);
		}
	}

	ConfigCompiler::AddIncludeSearchDir(Application::GetPkgDataDir());

	if (vm.count("include")) {
		BOOST_FOREACH(const String& includePath, vm["include"].as<vector<String> >()) {
			ConfigCompiler::AddIncludeSearchDir(includePath);
		}
	}

	if (vm.count("config") == 0) {
		Logger::Write(LogCritical, "icinga-app", "You need to specify at least one config file (using the --config option).");

		return EXIT_FAILURE;
	}

	try {
		DynamicObject::BeginTx();

		/* load config files */
		BOOST_FOREACH(const String& configPath, vm["config"].as<vector<String> >()) {
			String configFile = argv[2];
			vector<ConfigItem::Ptr> items;
			vector<ConfigType::Ptr> types;
		
			ConfigCompiler::CompileFile(configFile, &items, &types);

			Logger::Write(LogInformation, "icinga-app", "Registering config types...");
			BOOST_FOREACH(const ConfigType::Ptr& type, types) {
				type->Commit();
			}
		
			Logger::Write(LogInformation, "icinga-app", "Executing config items...");  
			BOOST_FOREACH(const ConfigItem::Ptr& item, items) {
				item->Commit();
			}

			Logger::Write(LogInformation, "icinga-app", "Validating config items...");	
			DynamicType::Ptr type;
			BOOST_FOREACH(tie(tuples::ignore, type), DynamicType::GetTypes()) {
				ConfigType::Ptr ctype = ConfigType::GetByName(type->GetName());
			
				if (!ctype) {
					Logger::Write(LogWarning, "icinga-app", "No config type found for type '" + type->GetName() + "'");

					continue;
				}

				DynamicObject::Ptr object;
				BOOST_FOREACH(tie(tuples::ignore, object), type->GetObjects()) {
					ctype->ValidateObject(object);
				}
			}
		}
		
		DynamicObject::FinishTx();
	} catch (const exception& ex) {
		Logger::Write(LogCritical, "icinga-app", "Configuration error: " + String(ex.what()));
		return EXIT_FAILURE;
	}

	Application::Ptr app = Application::GetInstance();

	if (!app)
		throw_exception(runtime_error("Configuration must create an Application object."));

	if (vm.count("validate")) {
		Logger::Write(LogInformation, "icinga-app", "Terminating as requested by --validate.");
		return EXIT_SUCCESS;
	}

	if (vm.count("daemonize")) {
		Logger::Write(LogInformation, "icinga", "Daemonizing.");
		Utility::Daemonize();
	}

	return app->Run();
}

