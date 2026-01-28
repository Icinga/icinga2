// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef STATSFUNCTION_H
#define STATSFUNCTION_H

#include "base/i2-base.hpp"
#include "base/function.hpp"

namespace icinga
{

#define REGISTER_STATSFUNCTION(name, callback) \
	REGISTER_FUNCTION(StatsFunctions, name, callback, "status:perfdata")

}

#endif /* STATSFUNCTION_H */
