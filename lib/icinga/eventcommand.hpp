// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EVENTCOMMAND_H
#define EVENTCOMMAND_H

#include "icinga/eventcommand-ti.hpp"
#include "icinga/checkable.hpp"

namespace icinga
{

/**
 * An event handler command.
 *
 * @ingroup icinga
 */
class EventCommand final : public ObjectImpl<EventCommand>
{
public:
	DECLARE_OBJECT(EventCommand);
	DECLARE_OBJECTNAME(EventCommand);

	static thread_local EventCommand::Ptr ExecuteOverride;

	void Execute(const Checkable::Ptr& checkable,
		const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false);
};

}

#endif /* EVENTCOMMAND_H */
