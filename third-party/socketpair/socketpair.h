/* socketpair.h
 * Copyright 2007 by Nathan C. Myers <ncm@cantrip.org>; some rights reserved.
 * This code is Free Software.  It may be copied freely, in original or
 * modified form, subject only to the restrictions that (1) the author is
 * relieved from all responsibilities for any use for any purpose, and (2)
 * this copyright notice must be retained, unchanged, in its entirety.  If
 * for any reason the author might be held responsible for any consequences
 * of copying or use, license is withheld.
 */

#ifndef SOCKETPAIR_H
#define SOCKETPAIR_H

#include "base/visibility.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef I2_SOCKETPAIR_BUILD
#       define I2_SOCKETPAIR_API I2_EXPORT
#else
#       define I2_SOCKETPAIR_API I2_IMPORT
#endif /* I2_SOCKETPAIR_BUILD */

#ifdef _WIN32
I2_SOCKETPAIR_API int dumb_socketpair(SOCKET socks[2], int make_overlapped);
#else /* _WIN32 */
I2_SOCKETPAIR_API int dumb_socketpair(int socks[2], int dummy);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SOCKETPAIR_H */

