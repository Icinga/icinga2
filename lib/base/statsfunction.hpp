/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STATSFUNCTION_H
#define STATSFUNCTION_H

#include "base/function.hpp"
#include "base/i2-base.hpp"

namespace icinga
{

#define REGISTER_STATSFUNCTION(name, callback) \
	REGISTER_FUNCTION(StatsFunctions, name, callback, "status:perfdata")

}

#endif /* STATSFUNCTION_H */
