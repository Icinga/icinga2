// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef NUMBER_H
#define NUMBER_H

#include "base/i2-base.hpp"
#include "base/object.hpp"

namespace icinga {

class Value;

/**
 * Number class.
 */
class Number
{
public:
	static Object::Ptr GetPrototype();

private:
	Number();
};

}

#endif /* NUMBER_H */
