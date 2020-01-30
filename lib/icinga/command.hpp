/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "icinga/i2-icinga.hpp"
#include "icinga/command-ti.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * A command.
 *
 * @ingroup icinga
 */
class Command : public ObjectImpl<Command>
{
public:
	DECLARE_OBJECT(Command);

	//virtual Dictionary::Ptr Execute(const Object::Ptr& context) = 0;

	void Validate(int types, const ValidationUtils& utils) override;
};

}
