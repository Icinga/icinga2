/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "config/configcompilercontext.h"
#include "config/configcompiler.h"
#include "config/configitembuilder.h"
#include "base/application.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include "base/exception.h"
#include "base/convert.h"
#include <boost/program_options.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

#ifndef _WIN32
#	include <ltdl.h>
#	include <sys/types.h>
#	include <pwd.h>
#	include <grp.h>
#endif /* _WIN32 */

using namespace icinga;
namespace po = boost::program_options;

static po::variables_map g_AppParams;

static bool LoadConfigFiles(bool validateOnly)
{
	ConfigCompilerContext::GetInstance()->Reset();

	BOOST_FOREACH(const String& configPath, g_AppParams["config"].as<std::vector<String> >()) {
		ConfigCompiler::CompileFile(configPath);
	}

	String name, fragment;
	BOOST_FOREACH(boost::tie(name, fragment), ConfigFragmentRegistry::GetInstance()->GetItems()) {
		ConfigCompiler::CompileText(name, fragment);
	}

	ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>();
	builder->SetType(Application::GetApplicationType());
	builder->SetName("application");
	ConfigItem::Ptr item = builder->Compile();
	item->Register();

	bool result = ConfigItem::ActivateItems(validateOnly);

	int warnings = 0, errors = 0;

	BOOST_FOREACH(const ConfigCompilerMessage& message, ConfigCompilerContext::GetInstance()->GetMessages()) {
		if (message.Error) {
			Log(LogCritical, "config", "Config error: " + message.Text);
			errors++;
		} else {
			Log(LogWarning, "config", "Config warning: " + message.Text);
			warnings++;
		}
	}

	if (warnings > 0 || errors > 0) {
		LogSeverity severity;

		if (errors == 0)
			severity = LogWarning;
		else
			severity = LogCritical;

		Log(severity, "config", Convert::ToString(errors) + " errors, " + Convert::ToString(warnings) + " warnings.");
	}

	if (!result)
		return false;

	ConfigItem::DiscardItems();
	ConfigType::DiscardTypes();

	return true;
}

static void SigHupHandler(int)
{
	Application::RequestRestart();
}

static bool Daemonize(const String& stderrFile)
{
#ifndef _WIN32
	pid_t pid = fork();
	if (pid == -1) {
		return false;
	}

	if (pid)
		exit(0);

	int fdnull = open("/dev/null", O_RDWR);
	if (fdnull > 0) {
		if (fdnull != 0)
			dup2(fdnull, 0);

		if (fdnull != 1)
			dup2(fdnull, 1);

		if (fdnull > 2)
			close(fdnull);
	}

	const char *errPath = "/dev/null";

	if (!stderrFile.IsEmpty())
		errPath = stderrFile.CStr();

	int fderr = open(errPath, O_WRONLY | O_APPEND);

	if (fderr < 0 && errno == ENOENT)
		fderr = open(errPath, O_CREAT | O_WRONLY | O_APPEND, 0600);

	if (fderr > 0) {
		if (fderr != 2)
			dup2(fderr, 2);

		if (fderr > 2)
			close(fderr);
	}

	pid_t sid = setsid();
	if (sid == -1) {
		return false;
	}
#endif

	return true;
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

	/* Set thread title. */
	Utility::SetThreadName("Main Thread", false);

	/* Set command-line arguments. */
	Application::SetArgC(argc);
	Application::SetArgV(argv);

	/* Install exception handlers to make debugging easier. */
	Application::InstallExceptionHandlers();

#ifdef ICINGA_PREFIX
	Application::SetPrefixDir(ICINGA_PREFIX);
#else /* ICINGA_PREFIX */
	Application::SetPrefixDir(".");
#endif /* ICINGA_PREFIX */

#ifdef ICINGA_LOCALSTATEDIR
	Application::SetLocalStateDir(ICINGA_LOCALSTATEDIR);
#else /* ICINGA_LOCALSTATEDIR */
	Application::SetLocalStateDir("./var");
#endif /* ICINGA_LOCALSTATEDIR */

#ifdef ICINGA_PKGLIBDIR
	Application::SetPkgLibDir(ICINGA_PKGLIBDIR);
#else /* ICINGA_PKGLIBDIR */
	Application::SetPkgLibDir(".");
#endif /* ICINGA_PKGLIBDIR */

#ifdef ICINGA_PKGDATADIR
	Application::SetPkgDataDir(ICINGA_PKGDATADIR);
#else /* ICINGA_PKGDATADIR */
	Application::SetPkgDataDir(".");
#endif /* ICINGA_PKGDATADIR */

	Application::SetStatePath(Application::GetLocalStateDir() + "/lib/icinga2/icinga2.state");
	Application::SetPidPath(Application::GetLocalStateDir() + "/run/icinga2/icinga2.pid");

	Application::SetApplicationType("IcingaApplication");

	po::options_description desc("Supported options");
	desc.add_options()
		("help", "show this help message")
		("version,V", "show version information")
		("library,l", po::value<std::vector<String> >(), "load a library")
		("include,I", po::value<std::vector<String> >(), "add include search directory")
		("config,c", po::value<std::vector<String> >(), "parse a configuration file")
		("validate,C", "exit after validating the configuration")
		("debug,x", "enable debugging")
		("daemonize,d", "detach from the controlling terminal")
		("errorlog,e", po::value<String>(), "log fatal errors to the specified log file (only works in combination with --daemonize)")
#ifndef _WIN32
		("user,u", po::value<String>(), "user to run Icinga as")
		("group,g", po::value<String>(), "group to run Icinga as")
#endif
	;

	try {
		po::store(po::parse_command_line(argc, argv, desc), g_AppParams);
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Error while parsing command-line options: " << ex.what();
		Log(LogCritical, "icinga-app", msgbuf.str());
		return EXIT_FAILURE;
	}

	po::notify(g_AppParams);

#ifndef _WIN32
	if (g_AppParams.count("group")) {
		String group = g_AppParams["group"].as<String>();

		errno = 0;
		struct group *gr = getgrnam(group.CStr());

		if (!gr) {
			if (errno == 0) {
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid group specified: " + group));
			} else {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("getgrnam")
					<< boost::errinfo_errno(errno));
			}
		}

		if (setgid(gr->gr_gid) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("setgid")
				<< boost::errinfo_errno(errno));
		}
	}

	if (g_AppParams.count("user")) {
		String user = g_AppParams["user"].as<String>();

		errno = 0;
		struct passwd *pw = getpwnam(user.CStr());

		if (!pw) {
			if (errno == 0) {
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid user specified: " + user));
			} else {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("getpwnam")
					<< boost::errinfo_errno(errno));
			}
		}

		if (setuid(pw->pw_uid) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("setuid")
				<< boost::errinfo_errno(errno));
		}
	}
#endif /* _WIN32 */

	if (g_AppParams.count("debug"))
		Application::SetDebugging(true);

	if (g_AppParams.count("help") || g_AppParams.count("version")) {
		String appName = Utility::BaseName(argv[0]);

		if (appName.GetLength() > 3 && appName.SubStr(0, 3) == "lt-")
			appName = appName.SubStr(3, appName.GetLength() - 3);

		std::cout << appName << " " << "- The Icinga 2 network monitoring daemon.";

		if (g_AppParams.count("version")) {
			std::cout  << " (Version: " << Application::GetVersion() << ")";
			std::cout << std::endl
				  << "Copyright (c) 2012-2013 Icinga Development Team (http://www.icinga.org)" << std::endl
				  << "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl2.html>" << std::endl
				  << "This is free software: you are free to change and redistribute it." << std::endl
				  << "There is NO WARRANTY, to the extent permitted by law.";
		}

		std::cout << std::endl;

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

	Log(LogInformation, "icinga-app", "Icinga application loader (version: " + Application::GetVersion() + ")");

	String searchDir = Application::GetPkgLibDir();
	Log(LogInformation, "base", "Adding library search dir: " + searchDir);

#ifdef _WIN32
	SetDllDirectory(searchDir.CStr());
#else /* _WIN32 */
	lt_dladdsearchdir(searchDir.CStr());
#endif /* _WIN32 */

	(void) Utility::LoadExtensionLibrary("icinga");

	if (g_AppParams.count("library")) {
		BOOST_FOREACH(const String& libraryName, g_AppParams["library"].as<std::vector<String> >()) {
			(void) Utility::LoadExtensionLibrary(libraryName);
		}
	}

	ConfigCompiler::AddIncludeSearchDir(Application::GetPkgDataDir());

	if (g_AppParams.count("include")) {
		BOOST_FOREACH(const String& includePath, g_AppParams["include"].as<std::vector<String> >()) {
			ConfigCompiler::AddIncludeSearchDir(includePath);
		}
	}

	if (g_AppParams.count("config") == 0) {
		Log(LogCritical, "icinga-app", "You need to specify at least one config file (using the --config option).");

		return EXIT_FAILURE;
	}

	if (g_AppParams.count("daemonize")) {
		String errorLog;

		if (g_AppParams.count("errorlog"))
			errorLog = g_AppParams["errorlog"].as<String>();

		Daemonize(errorLog);
	}

	bool validateOnly = g_AppParams.count("validate");

	if (!LoadConfigFiles(validateOnly))
		return EXIT_FAILURE;

	if (validateOnly) {
		Log(LogInformation, "icinga-app", "Finished validating the configuration file(s).");
		return EXIT_SUCCESS;
	}

#ifndef _WIN32
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &SigHupHandler;
	sigaction(SIGHUP, &sa, NULL);
#endif /* _WIN32 */

	int rc = Application::GetInstance()->Run();

#ifdef _DEBUG
	exit(rc);
#else /* _DEBUG */
	_exit(rc); // Yay, our static destructors are pretty much beyond repair at this point.
#endif /* _DEBUG */
}
