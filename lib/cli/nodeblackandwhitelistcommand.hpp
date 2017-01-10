/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef BLACKANDWHITELISTCOMMAND_H
#define BLACKANDWHITELISTCOMMAND_H

#include "cli/clicommand.hpp"
#include <boost/algorithm/string/case_conv.hpp>

namespace icinga
{

enum BlackAndWhitelistCommandType
{
	BlackAndWhitelistCommandAdd,
	BlackAndWhitelistCommandRemove,
	BlackAndWhitelistCommandList
};

/**
 * The "repository <type> <add/remove/list>" command.
 *
 * @ingroup cli
 */
class I2_CLI_API BlackAndWhitelistCommand : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(BlackAndWhitelistCommand);

	BlackAndWhitelistCommand(const String& type, BlackAndWhitelistCommandType command);

	virtual String GetDescription(void) const override;
	virtual String GetShortDescription(void) const override;
	virtual bool IsDeprecated(void) const override;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
	    boost::program_options::options_description& hiddenDesc) const override;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

private:
	String m_Type;
	BlackAndWhitelistCommandType m_Command;
};

#define REGISTER_BLACKANDWHITELIST_CLICOMMAND(type) \
	INITIALIZE_ONCE([]() { \
		String ltype = #type; \
		boost::algorithm::to_lower(ltype); \
\
		std::vector<String> name; \
		name.push_back("node"); \
		name.push_back(ltype); \
		name.push_back("add"); \
		CLICommand::Register(name, new BlackAndWhitelistCommand(#type, BlackAndWhitelistCommandAdd)); \
\
		name[2] = "remove"; \
		CLICommand::Register(name, new BlackAndWhitelistCommand(#type, BlackAndWhitelistCommandRemove)); \
\
		name[2] = "list"; \
		CLICommand::Register(name, new BlackAndWhitelistCommand(#type, BlackAndWhitelistCommandList)); \
	})

}

#endif /* BLACKANDWHITELISTCOMMAND_H */
