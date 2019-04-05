/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CONSOLECOMMAND_H
#define CONSOLECOMMAND_H

#include "cli/clicommand.hpp"
#include "base/exception.hpp"
#include "base/scriptframe.hpp"
#include "base/tlsstream.hpp"
#include "remote/url.hpp"


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
	mutable boost::mutex m_Mutex;
	mutable boost::condition_variable m_CV;

	static std::shared_ptr<AsioTlsStream> Connect(const String& host, const String& port);

	static Value ExecuteScript(const String& session, const std::shared_ptr<AsioTlsStream>& tlsStream,
		const Url::Ptr& url, const String& command, bool sandboxed);

	static Dictionary::Ptr SendRequest(const std::shared_ptr<AsioTlsStream>& tlsStream, const Url::Ptr& url);

	static void ExecuteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
		bool& ready, const boost::exception_ptr& eptr, const Value& result, Value& resultOut,
		boost::exception_ptr& eptrOut);
	static void AutocompleteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
		bool& ready, const boost::exception_ptr& eptr, const Array::Ptr& result, Array::Ptr& resultOut);

#ifdef HAVE_EDITLINE
	static char *ConsoleCompleteHelper(const char *word, int state);
#endif /* HAVE_EDITLINE */

	static void BreakpointHandler(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di);

};

}

#endif /* CONSOLECOMMAND_H */
