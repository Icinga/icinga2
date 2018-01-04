/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef CONSOLECOMMAND_H
#define CONSOLECOMMAND_H

#include "cli/clicommand.hpp"
#include "base/exception.hpp"
#include "base/scriptframe.hpp"

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

	virtual String GetDescription() const override;
	virtual String GetShortDescription() const override;
	virtual ImpersonationLevel GetImpersonationLevel() const override;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

	static int RunScriptConsole(ScriptFrame& scriptFrame, const String& addr = String(),
		const String& session = String(), const String& commandOnce = String(), const String& commandOnceFileName = String(),
		bool syntaxOnly = false);

private:
	mutable boost::mutex m_Mutex;
	mutable boost::condition_variable m_CV;

	static void ExecuteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
		bool& ready, boost::exception_ptr eptr, const Value& result, Value& resultOut,
		boost::exception_ptr& eptrOut);
	static void AutocompleteScriptCompletionHandler(boost::mutex& mutex, boost::condition_variable& cv,
		bool& ready, boost::exception_ptr eptr, const Array::Ptr& result, Array::Ptr& resultOut);

#ifdef HAVE_EDITLINE
	static char *ConsoleCompleteHelper(const char *word, int state);
#endif /* HAVE_EDITLINE */

	static void BreakpointHandler(ScriptFrame& frame, ScriptError *ex, const DebugInfo& di);

};

}

#endif /* CONSOLECOMMAND_H */
