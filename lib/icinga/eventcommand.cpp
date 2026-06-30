// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/eventcommand.hpp"
#include "icinga/eventcommand-ti.cpp"

using namespace icinga;

REGISTER_TYPE(EventCommand);

thread_local EventCommand::Ptr EventCommand::ExecuteOverride;

void EventCommand::Execute(const Checkable::Ptr& checkable,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	GetExecute()->Invoke({
		checkable,
		resolvedMacros,
		useResolvedMacros
	});
}
