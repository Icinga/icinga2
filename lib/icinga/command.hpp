// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef COMMAND_H
#define COMMAND_H

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

#endif /* COMMAND_H */
