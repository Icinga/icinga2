/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#include "perl/languageperl.hpp"
#include "base/application.hpp"
#include "base/configuration.hpp"
#include "base/logger.hpp"

using namespace icinga;

LanguagePerl::LanguagePerl()
{
	Initialize();
}

LanguagePerl::~LanguagePerl()
{
	perl_destruct(m_Interpreter);
	perl_free(m_Interpreter);
}

String LanguagePerl::GetEmbedPath()
{
	return Configuration::LibDir + "/icinga2/perl/embed.pl";
}

void LanguagePerl::Initialize()
{
	String embedFile = Configuration::LibDir;
	int argc = 2;
	auto **argv = new char *[2];
	argv[0] = strdup(embedFile.CStr());

	PERL_SYS_INIT3(&argc, &argv, nullptr); //TODO: Figure out which 'env' needs to be passed here.


}


void LanguagePerl::Terminate()
{
	PERL_SYS_TERM();
}

void LanguagePerl::Run(const std::vector<String>& arguments)
{
	/*
	char *args[5] = {"", DO_CLEAN, "", "", nullptr };
	SV *plugin_handler_cr = nullptr;

	String fileName = arguments[0]; // TODO: Check whether a) the file has the header epn, and is actually the plugin. Fallback to arguments[1] should be provide.

	args[0] = const_cast<char *>(fileName.CStr());

	// build argv
	auto **argv = new char *[arguments.size() + 1];

	for (unsigned int i = 0; i < arguments.size(); i++) {
		String arg = arguments[i];
		argv[i] = strdup(arg.CStr());
	}

	argv[arguments.size()] = nullptr;


	perl_run(m_Interpreter);

	// https://github.com/Icinga/icinga-core/blob/master/contrib/mini_epn.c




	// free arguments
	for (int i = 0; argv[i]; i++)
		free(argv[i]);

	delete[] argv;
*/
}

int LanguagePerl::RunStandalone(const String& fileName, const String& arguments)
{
	String embedFile = GetEmbedPath();
	char *embedding[] = { const_cast<char *>(""), const_cast<char *>(embedFile.CStr()) };

	int argc = 2;
	char **env; //TODO: Retrieve from main().

	char *args[] = {
		const_cast<char *>(""),
		const_cast<char *>("0"),
		const_cast<char *>(""),
		const_cast<char *>(""),
		nullptr
	};

	PERL_SYS_INIT3(&argc, (char ***)(embedding), &env);

	if ((m_Interpreter = perl_alloc()) == nullptr) {
		Log(LogCritical, "LanguagePerl")
			<< "Error: Could not allocate memory for embedded perl interpreter!";
		return 1;
	}

	perl_construct(m_Interpreter);

	int exitState = perl_parse(m_Interpreter, xs_init, 2, embedding, nullptr);

	if (!exitState) {
		exitState = perl_run(m_Interpreter);

		/**
		 * All of those strange functions with sv in their names help convert Perl scalars to C types.
		 * https://perldoc.perl.org/perlguts.html#Datatypes
		 */

		SV* plugin_hndlr_cr;
		STRLEN n_a;
		int count = 0;
		int pcloseResult;
		String output;

		/**
		 * Parameters are passed to the Perl subroutine using the Perl stack.
		 * This is the purpose of the code beginning with the line dSP and ending with the line PUTBACK.
		 * The dSP declares a local copy of the stack pointer. This local copy should always be accessed as SP.
		 */
		dSP; // Parameter stack begin

		args[0] = const_cast<char *>(fileName.CStr());
		args[2] = const_cast<char *>("");
		args[3] = const_cast<char *>(arguments.CStr());

		/**
		 * Call the Perl interpreter to compile and optionally cache the command.
		 *
		 * sv_2mortal() creates temporary copies which we'll have to clean later with FREETMPS
		 */
		ENTER;
		SAVETMPS;
		PUSHMARK(SP); // Stack pointer.

		XPUSHs(sv_2mortal(newSVpv(args[0], 0)));
		XPUSHs(sv_2mortal(newSVpv(args[1], 0)));
		XPUSHs(sv_2mortal(newSVpv(args[2], 0)));
		XPUSHs(sv_2mortal(newSVpv(args[3], 0)));

		PUTBACK; // Parameter stack end

		/**
		 * Evaluate and compile the perl file
		 *
		 * G_SCALAR: The @_ array will be created and the value returned will still exist after the call to call_pv.
		 * G_EVAL: Add an eval trap for die() termination events.
		 *
		 * https://perldoc.perl.org/perlcall.html
		 */
		count = call_pv("Embed::Persistent::eval_file", G_SCALAR | G_EVAL);

		/**
		 * The purpose of the macro SPAGAIN is to refresh the local copy of the stack pointer.
		 * This is necessary because it is possible that the memory allocated to the Perl stack
		 * has been reallocated during the call_pv call.
		 */
		SPAGAIN;

		/**
		 * Check for errors.
		 */
		if (SvTRUE(ERRSV)) {
			// Pop the return value from the stack and drop it in case of a failure
			(void) POPs;

			Log(LogCritical, "LanguagePerl")
				<< "Embedded Perl ran command '" << fileName << " " << arguments << "' " << SvPVX(ERRSV);

			return -2;
		}

		/**
  		 * Create a copy of the popped stack result scalar value.
  		 */
		plugin_hndlr_cr = newSVsv(POPs);

		/**
		 * Cleanup the stack
		 */
		PUTBACK;
		FREETMPS;
		LEAVE;

		/**
		 * Run the compiled package plugin stored in pluginHandlerCheckResult.
		 */
		ENTER;
		SAVETMPS;
		PUSHMARK(SP); // Opening bracket for arguments on a callback

		XPUSHs(sv_2mortal(newSVpv(args[0], 0)));
		XPUSHs(sv_2mortal(newSVpv(args[1], 0)));
		XPUSHs(plugin_hndlr_cr);
		XPUSHs(sv_2mortal(newSVpv(args[3], 0)));

		PUTBACK; // Closing bracket for XSUB arguments.

		/*
		 * Run it.
		 */
		count = perl_call_pv("Embed::Persistent::run_package", G_EVAL | G_ARRAY);

		/*
		 * Refresh the stack pointer copy.
		 */
		SPAGAIN;

		/*
		 * Pop the string off the stack (POPpx) as output.
		 * Pop the integer off the stack (POPi) as result.
		 */
		output = POPpx;
		pcloseResult = POPi;

		Log(LogInformation, "LanguagePerl")
			<< "Embedded Perl Plugin return code '" << pcloseResult << "' and output: '" << output << "'.";

		/**
		 * Cleanup.
		 */
		PUTBACK;
		FREETMPS;
		LEAVE;
	}


	/**
	 * Uninitialize.
	 */
	PL_perl_destruct_level = 0;
	perl_destruct(m_Interpreter);
	perl_free(m_Interpreter);

	Application::Exit(exitState);
}
