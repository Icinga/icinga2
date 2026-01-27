// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/checkcommand.hpp"
#include "icinga/checkcommand-ti.cpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_TYPE(CheckCommand);

thread_local CheckCommand::Ptr CheckCommand::ExecuteOverride;

void CheckCommand::Execute(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	GetExecute()->Invoke({
		checkable,
		cr,
		producer,
		resolvedMacros,
		useResolvedMacros
	});
}
