/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef CONSOLEHANDLER_H
#define CONSOLEHANDLER_H

#include "remote/httphandler.hpp"
#include "base/scriptframe.hpp"

namespace icinga
{

struct I2_REMOTE_API ApiScriptFrame
{
	double Seen;
	int NextLine;
	std::map<String, String> Lines;
	Dictionary::Ptr Locals;

	ApiScriptFrame(void)
		: Seen(0), NextLine(1)
	{ }
};

class I2_REMOTE_API ConsoleHandler : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ConsoleHandler);

	virtual bool HandleRequest(const ApiUser::Ptr& user, HttpRequest& request, HttpResponse& response) override;

	static std::vector<String> GetAutocompletionSuggestions(const String& word, ScriptFrame& frame);

private:
	static bool ExecuteScriptHelper(HttpRequest& request, HttpResponse& response,
	    const String& command, const String& session, bool sandboxed);
	static bool AutocompleteScriptHelper(HttpRequest& request, HttpResponse& response,
	    const String& command, const String& session, bool sandboxed);

};

}

#endif /* CONSOLEHANDLER_H */
