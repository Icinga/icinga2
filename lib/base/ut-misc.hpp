/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_MISC_H
#define UT_MISC_H

#ifndef _WIN32
#	include <unistd.h>
#endif /* _WIN32 */

namespace icinga
{
namespace UT
{
namespace Aware
{

#ifndef _WIN32
	decltype(fork()) Fork();
#endif /* _WIN32 */

}
}
}

#endif /* UT_MISC_H */
