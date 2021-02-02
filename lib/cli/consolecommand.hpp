/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONSOLECOMMAND_H
#define CONSOLECOMMAND_H

#include "cli/clicommand.hpp"
#include "base/exception.hpp"
#include "base/scriptframe.hpp"
#include "base/tlsstream.hpp"
#include "remote/url.hpp"
#include <condition_variable>


namespace icinga
{

/**
 * The "console" CLI command.
 *
 * @ingroup cli
 */
class ConsoleCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(ConsoleCommand);

	static void StaticInitialize();

	String GetDescription() const override;
	String GetShortDescription() const override;
	ImpersonationLevel GetImpersonationLevel() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

	static int RunScriptConsole(ScriptFrame& scriptFrame, const String& connectAddr = String(),
		const String& session = String(), const String& commandOnce = String(), const String& commandOnceFileName = String(),
		bool syntaxOnly = false);

private:
	mutable std::mutex m_Mutex;
	mutable std::condition_variable m_CV;

	static Shared<AsioTlsStream>::Ptr Connect();

	static Value ExecuteScript(const String& session, const String& command, bool sandboxed);
	static Array::Ptr AutoCompleteScript(const String& session, const String& command, bool sandboxed);

	static Dictionary::Ptr SendRequest();

#ifdef HAVE_EDITLINE
	static char *ConsoleCompleteHelper(const char *word, int state);
#endif /* HAVE_EDITLINE */

	static void BreakpointHandler(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di);

};

}

#endif /* CONSOLECOMMAND_H */
