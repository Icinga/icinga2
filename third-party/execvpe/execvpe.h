/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXECVPE_H
#define EXECVPE_H

#ifndef _MSC_VER

#ifdef HAVE_EXECVPE

#include <unistd.h>

inline int icinga2_execvpe(const char *file, char *const argv[], char *const envp[])
{
	return execvpe(file, argv, envp);
}

#else /* HAVE_EXECVPE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int icinga2_execvpe(const char *file, char *const argv[], char *const envp[]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HAVE_EXECVPE */

#endif /* _MSC_VER */

#endif /* EXECVPE_H */
