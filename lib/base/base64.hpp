/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/i2-remote.hpp"
#include "base/string.hpp"

namespace icinga
{

/**
 * Base64
 *
 * @ingroup remote
 */
struct Base64
{
	static String Decode(const String& data);
	static String Encode(const String& data);
};

}
