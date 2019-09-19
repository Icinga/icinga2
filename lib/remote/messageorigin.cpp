/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/messageorigin.hpp"

using namespace icinga;

bool MessageOrigin::IsLocal() const
{
	return !FromClient;
}
