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

#ifndef REPOSITORYOBJECTCOMMAND_H
#define REPOSITORYOBJECTCOMMAND_H

#include "base/qstring.hpp"
#include "base/clicommand.hpp"

namespace icinga
{

enum RepositoryCommandType
{
	RepositoryCommandAdd,
	RepositoryCommandRemove,
	RepositoryCommandList
};

/**
 * The "repository <type> <add/remove/list>" command.
 *
 * @ingroup cli
 */
class RepositoryObjectCommand : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(RepositoryObjectCommand);

	RepositoryObjectCommand(const String& type, RepositoryCommandType command);

	virtual String GetDescription(void) const;
	virtual String GetShortDescription(void) const;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
	    boost::program_options::options_description& hiddenDesc) const;
	virtual std::vector<String> GetPositionalSuggestions(const String& word) const;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const;

private:
	String m_Type;
	RepositoryCommandType m_Command;
};

/**
 * Helper class for registering repository CLICommand implementation classes.
 *
 * @ingroup cli
 */
class I2_BASE_API RegisterRepositoryCLICommandHelper
{
public:
	RegisterRepositoryCLICommandHelper(const String& type);
};

#define REGISTER_REPOSITORY_CLICOMMAND(type) \
	namespace { namespace UNIQUE_NAME(cli) { \
		I2_EXPORT icinga::RegisterRepositoryCLICommandHelper l_RegisterRepositoryCLICommand(type); \
	} }

}

#endif /* REPOSITORYOBJECTCOMMAND_H */
