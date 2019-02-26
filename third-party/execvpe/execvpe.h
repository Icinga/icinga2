/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXECVPE_H
#define EXECVPE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _MSC_VER
int icinga2_execvpe(const char *file, char *const argv[], char *const envp[]);
#endif /* _MSC_VER */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EXECVPE_H */
