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

static po::variables_map g_AppParams;
static vector<ConfigItem::WeakPtr> g_ConfigItems;

#ifndef _WIN32
static bool g_ReloadConfig = false;
static Timer::Ptr g_ReloadConfigTimer;
#endif /* _WIN32 */

static bool LoadConfigFiles(void)
{
	set<ConfigItem::Ptr> allItems;

	try {
		BOOST_FOREACH(const String& configPath, g_AppParams["config"].as<vector<String> >()) {
			vector<ConfigItem::Ptr> items;
			vector<ConfigType::Ptr> types;

			ConfigCompiler::CompileFile(configPath, &items, &types);

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

			std::copy(items.begin(), items.end(), std::inserter(allItems, allItems.begin()));
		}

		BOOST_FOREACH(const ConfigItem::WeakPtr& witem, g_ConfigItems) {
			ConfigItem::Ptr item = witem.lock();

			/* Ignore this item if it's not active anymore */
			if (!item || ConfigItem::GetObject(item->GetType(), item->GetName()) != item)
				continue;

			/* Remove the object if it's not in the list of current items */
			if (allItems.find(item) == allItems.end())
				item->Unregister();
		}

		g_ConfigItems.clear();
		std::copy(allItems.begin(), allItems.end(), std::back_inserter(g_ConfigItems));

		return true;
	} catch (const exception& ex) {
		Logger::Write(LogCritical, "icinga-app", "Configuration error: " + String(ex.what()));
		return false;
	}

}

static void ReloadConfigTimerHandler(void)
{
	if (g_ReloadConfig) {
		Logger::Write(LogInformation, "icinga-app", "Received SIGHUP. Reloading config files.");
		LoadConfigFiles();
		g_ReloadConfig = false;
	}
}

static void SigHupHandler(int signum)
{
	assert(signum == SIGHUP);

	g_ReloadConfig = true;
}

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

	po::options_description desc("Supported options");
	desc.add_options()
		("help,h", "show this help message")
		("version,V", "show version information")
		("library,l", po::value<vector<String> >(), "load a library")
		("include,I", po::value<vector<String> >(), "add include search directory")
		("config,c", po::value<vector<String> >(), "parse a configuration file")
		("validate,v", "exit after validating the configuration")
		("debug", "enable debugging")
		("daemonize,d", "daemonize after reading the configuration files")
	;

	try {
		po::store(po::parse_command_line(argc, argv, desc), g_AppParams);
	} catch (const exception& ex) {
		stringstream msgbuf;
		msgbuf << "Error while parsing command-line options: " << ex.what();
		Logger::Write(LogCritical, "icinga-app", msgbuf.str());
		return EXIT_FAILURE;
	}

	po::notify(g_AppParams);

	if (g_AppParams.count("debug"))
		Application::SetDebugging(true);

	if (g_AppParams.count("help") || g_AppParams.count("version")) {
		std::cout << "Icinga application loader"
#ifndef _WIN32
			  << " (version: " << ICINGA_VERSION << ")"
#endif /* _WIN32 */
			  << std::endl
			  << "Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)" << std::endl
			  << "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl2.html>" << std::endl
			  << "This is free software: you are free to change and redistribute it." << std::endl
			  << "There is NO WARRANTY, to the extent permitted by law." << std::endl;

		if (g_AppParams.count("version"))
			return EXIT_SUCCESS;
	}

	if (g_AppParams.count("help")) {
		std::cout << std::endl
			  << desc << std::endl
			  << "Report bugs at <https://dev.icinga.org/>" << std::endl
			  << "Icinga home page: <http://www.icinga.org/>" << std::endl;
		return EXIT_SUCCESS;
	}

	Logger::Write(LogInformation, "icinga-app", "Icinga application loader"
#ifndef _WIN32
	    " (version: " ICINGA_VERSION ")"
#endif /* _WIN32 */
	    );

	Component::AddSearchDir(Application::GetPkgLibDir());

	Utility::LoadIcingaLibrary("icinga", false);

	if (g_AppParams.count("library")) {
		BOOST_FOREACH(const String& libraryName, g_AppParams["library"].as<vector<String> >()) {
			Utility::LoadIcingaLibrary(libraryName, false);
		}
	}

	ConfigCompiler::AddIncludeSearchDir(Application::GetPkgDataDir());

	if (g_AppParams.count("include")) {
		BOOST_FOREACH(const String& includePath, g_AppParams["include"].as<vector<String> >()) {
			ConfigCompiler::AddIncludeSearchDir(includePath);
		}
	}

	if (g_AppParams.count("config") == 0) {
		Logger::Write(LogCritical, "icinga-app", "You need to specify at least one config file (using the --config option).");

		return EXIT_FAILURE;
	}

	DynamicObject::BeginTx();

	if (!LoadConfigFiles())
		return EXIT_FAILURE;

	DynamicObject::FinishTx();

	Application::Ptr app = Application::GetInstance();

	if (!app)
		throw_exception(runtime_error("Configuration must create an Application object."));

	if (g_AppParams.count("validate")) {
		Logger::Write(LogInformation, "icinga-app", "Terminating as requested by --validate.");
		return EXIT_SUCCESS;
	}

	if (g_AppParams.count("daemonize")) {
		Logger::Write(LogInformation, "icinga", "Daemonizing.");
		Utility::Daemonize();
	}

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &SigHupHandler;
	sigaction(SIGHUP, &sa, NULL);

	g_ReloadConfigTimer = boost::make_shared<Timer>();
	g_ReloadConfigTimer->SetInterval(1);
	g_ReloadConfigTimer->OnTimerExpired.connect(boost::bind(&ReloadConfigTimerHandler));
	g_ReloadConfigTimer->Start();
#endif /* _WIN32 */

	return app->Run();
}

