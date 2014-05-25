#ifndef MMATCH_H
#define MMATCH_H

#include "base/visibility.hpp"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef I2_MMATCH_BUILD
#	define I2_MMATCH_API I2_EXPORT
#else
#	define I2_MMATCH_API I2_IMPORT
#endif /* I2_MMATCH_BUILD */

I2_MMATCH_API int mmatch(const char *old_mask, const char *new_mask);
I2_MMATCH_API int match(const char *mask, const char *str);
I2_MMATCH_API char *collapse(char *pattern);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MMATCH_H */
