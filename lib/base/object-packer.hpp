// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OBJECT_PACKER
#define OBJECT_PACKER

#include "base/i2-base.hpp"

namespace icinga
{

class String;
class Value;

String PackObject(const Value& value);

}

#endif /* OBJECT_PACKER */
