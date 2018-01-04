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

#ifndef PLUGINUTILITY_H
#define PLUGINUTILITY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkable.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include <vector>

namespace icinga
{

struct ProcessResult;

/**
 * Utility functions for plugin-based checks.
 *
 * @ingroup icinga
 */
class PluginUtility
{
public:
	static void ExecuteCommand(const Command::Ptr& commandObj, const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, const MacroProcessor::ResolverList& macroResolvers,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros,
		const std::function<void(const Value& commandLine, const ProcessResult&)>& callback = std::function<void(const Value& commandLine, const ProcessResult&)>());

	static ServiceState ExitStatusToState(int exitStatus);
	static std::pair<String, String> ParseCheckOutput(const String& output);

	static Array::Ptr SplitPerfdata(const String& perfdata);
	static String FormatPerfdata(const Array::Ptr& perfdata);

private:
	PluginUtility();
};

}

#endif /* PLUGINUTILITY_H */
