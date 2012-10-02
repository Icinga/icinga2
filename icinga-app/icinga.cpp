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
#ifdef HAVE_BACKTRACE_SYMBOLS
#	include <execinfo.h>
#endif /* HAVE_BACKTRACE_SYMBOLS */

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION VERSION ", " GIT_MESSAGE

#	include <ltdl.h>
#endif /* _WIN32 */

using namespace icinga;

/**
 * Handler for unhandled exceptions.
 *
 */
static void exception_handler(void)
{
	static bool rethrow = true;

	try {
		rethrow = false;
		throw;
	} catch (const std::exception& ex) {
		std::cerr << std::endl;
		std::cerr << "Unhandled exception of type "
			  << Utility::GetTypeName(typeid(ex))
			  << std::endl;
		std::cerr << "Diagnostic Information: "
			  << ex.what()
			  << std::endl;
	}

#ifdef HAVE_BACKTRACE_SYMBOLS
	void *frames[50];
	int framecount = backtrace(frames, sizeof(frames) / sizeof(frames[0]));

	char **messages = backtrace_symbols(frames, framecount);

	std::cerr << std::endl << "Stacktrace:" << std::endl;

	for (int i = 0; i < framecount && messages != NULL; ++i) {
		String message = messages[i];

		char *sym_begin = strchr(messages[i], '(');

		if (sym_begin != NULL) {
			char *sym_end = strchr(sym_begin, '+');

			if (sym_end != NULL) {
				String sym = String(sym_begin + 1, sym_end);
				String sym_demangled = Utility::DemangleSymbolName(sym);

				if (sym_demangled.IsEmpty())
					sym_demangled = "<unknown function>";

				message = String(messages[i], sym_begin) + ": " + sym_demangled + " (" + String(sym_end);
			}
		}

        	std::cerr << "\t(" << i << ") " << message << std::endl;
	}

	free(messages);

	std::cerr << std::endl;
#endif /* HAVE_BACKTRACE_SYMBOLS */

	abort();
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
	std::set_terminate(exception_handler);

#ifndef _WIN32
	LTDL_SET_PRELOADED_SYMBOLS();
#endif /* _WIN32 */

#ifndef _WIN32
	lt_dlinit();
#endif /* _WIN32 */

	/* This must be done before calling any other functions
	 * in the base library. */
	Application::SetMainThread();

#ifdef ICINGA_PREFIX
	Application::SetPrefixDir(ICINGA_PREFIX);
#endif /* ICINGA_PREFIX */

#ifdef ICINGA_LOCALSTATEDIR
	Application::SetLocalStateDir(ICINGA_LOCALSTATEDIR);
#endif /* ICINGA_LOCALSTATEDIR */

#ifdef ICINGA_PKGLIBDIR
	Application::SetPkgLibDir(ICINGA_PKGLIBDIR);
#endif /* ICINGA_PKGLIBDIR */

	Logger::Write(LogInformation, "icinga", "Icinga application loader"
#ifndef _WIN32
		" (version: " ICINGA_VERSION ")"
#endif /* _WIN32 */
	);

	if (argc < 3 || strcmp(argv[1], "-c") != 0) {
		stringstream msgbuf;
		msgbuf << "Syntax: " << argv[0] << " -c <config-file> ...";    
		Logger::Write(LogInformation, "base", msgbuf.str());
		return EXIT_FAILURE;
	}

	Component::AddSearchDir(Application::GetPkgLibDir());

	try {
		DynamicObject::BeginTx();

		/* load config file */
		String configFile = argv[2];
		vector<ConfigItem::Ptr> configItems = ConfigCompiler::CompileFile(configFile);

		Logger::Write(LogInformation, "icinga", "Executing config items...");  

		BOOST_FOREACH(const ConfigItem::Ptr& item, configItems) {
			item->Commit();
		}

		DynamicObject::FinishTx();
	} catch (const exception& ex) {
		Logger::Write(LogCritical, "icinga", "Configuration error: " + String(ex.what()));
		return EXIT_FAILURE;
	}

	Application::Ptr app = Application::GetInstance();

	if (!app)
		throw_exception(runtime_error("Configuration must create an Application object."));

	/* The application class doesn't need to know about the "-c configFile"
	 * command-line arguments. */
	return app->Run(argc - 2, &(argv[2]));
}

