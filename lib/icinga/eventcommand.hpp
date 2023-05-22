/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
