#ifndef MMATCH_H
#define MMATCH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int mmatch(const char *old_mask, const char *new_mask);
int match(const char *mask, const char *str);
char *collapse(char *pattern);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MMATCH_H */
