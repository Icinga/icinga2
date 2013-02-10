/* ----------------------------------------------------------------------------
   (c) The University of Glasgow 2004

   Interface for code in execvpe.c
   ------------------------------------------------------------------------- */

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifndef HAVE_EXECVPE
#	ifdef __cplusplus
extern "C" {
#	endif /* __cplusplus */

#	if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(_WIN32)
#		ifndef __QNXNTO__
extern int execvpe(char *name, char *const argv[], char **envp);
#	endif
extern void pPrPr_disableITimers (void);
#	endif

#	ifdef __cplusplus
}
#	endif /* __cplusplus */
#endif /* HAVE_EXECVPE */
