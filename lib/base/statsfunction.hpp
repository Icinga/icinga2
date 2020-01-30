/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/function.hpp"

namespace icinga
{

#define REGISTER_STATSFUNCTION(name, callback) \
	REGISTER_FUNCTION(StatsFunctions, name, callback, "status:perfdata")

}
