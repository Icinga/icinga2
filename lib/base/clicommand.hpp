/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef CLICOMMAND_H
#define CLICOMMAND_H

#include "base/i2-base.hpp"
#include "base/value.hpp"
#include "base/utility.hpp"
#include <vector>
#include <boost/program_options.hpp>

namespace icinga
{

I2_BASE_API std::vector<String> GetBashCompletionSuggestions(const String& type, const String& word);
I2_BASE_API std::vector<String> GetFieldCompletionSuggestions(const Type *type, const String& word);

/**
 * A CLI command.
 *
 * @ingroup base
 */
class I2_BASE_API CLICommand : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(CLICommand);

	typedef std::vector<String>(*ArgumentCompletionCallback)(const String&, const String&);

	virtual String GetDescription(void) const = 0;
	virtual String GetShortDescription(void) const = 0;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
	    boost::program_options::options_description& hiddenDesc) const;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const = 0;
	virtual std::vector<String> GetArgumentSuggestions(const String& argument, const String& word) const;
	virtual std::vector<String> GetPositionalSuggestions(const String& word) const;

	static CLICommand::Ptr GetByName(const std::vector<String>& name);
	static void Register(const std::vector<String>& name, const CLICommand::Ptr& command);
	static void Unregister(const std::vector<String>& name);

	static bool ParseCommand(int argc, char **argv, boost::program_options::options_description& visibleDesc,
	    boost::program_options::options_description& hiddenDesc,
	    boost::program_options::positional_options_description& positionalDesc,
	    boost::program_options::variables_map& vm, String& cmdname,
	   CLICommand::Ptr& command, bool autocomplete);

	static void ShowCommands(int argc, char **argv,
	    boost::program_options::options_description *visibleDesc = NULL,
	    boost::program_options::options_description *hiddenDesc = NULL,
	    ArgumentCompletionCallback globalArgCompletionCallback = NULL,
	    bool autocomplete = false, int autoindex = -1);
};

/**
 * Helper class for registering CLICommand implementation classes.
 *
 * @ingroup base
 */
class I2_BASE_API RegisterCLICommandHelper
{
public:
	RegisterCLICommandHelper(const String& name, const CLICommand::Ptr& command);
};

#define REGISTER_CLICOMMAND(name, klass) \
	namespace { namespace UNIQUE_NAME(cli) { \
		I2_EXPORT icinga::RegisterCLICommandHelper l_RegisterCLICommand(name, make_shared<klass>()); \
	} }

}

#endif /* CLICOMMAND_H */
