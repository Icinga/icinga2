/* Copyright (C) 1991,92, 1995-99, 2002, 2004, 2005, 2007, 2009
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
#include <alloca.h>
#endif /* !__FreeBSD__ && !__OpenBSD__ && !__NetBSD__  */
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "execvpe.h"

#if !defined(_MSC_VER) && !defined(HAVE_EXECVPE)

/* The file is accessible but it is not an executable file.  Invoke
   the shell to interpret it as a script.  */
static void
scripts_argv (const char *file, char *const argv[], int argc, char **new_argv)
{
  /* Construct an argument list for the shell.  */
  new_argv[0] = (char *) "/bin/sh";
  new_argv[1] = (char *) file;
  while (argc > 1)
    {
      new_argv[argc] = argv[argc - 1];
      --argc;
    }
}


/* Execute FILE, searching in the `PATH' environment variable if it contains
   no slashes, with arguments ARGV and environment from ENVP.  */
int
icinga2_execvpe (const char *file, char *const argv[], char *const envp[])
{
  if (*file == '\0')
    {
      /* We check the simple case first. */
      errno = ENOENT;
      return -1;
    }

  if (strchr (file, '/') != NULL)
    {
      /* Don't search when it contains a slash.  */
      execve (file, argv, envp);

      if (errno == ENOEXEC)
        {
          /* Count the arguments.  */
          int argc = 0;
          while (argv[argc++])
            ;
          size_t len = (argc + 1) * sizeof (char *);
          char **script_argv;
          void *ptr = NULL;
          script_argv = alloca (len);

          if (script_argv != NULL)
            {
              scripts_argv (file, argv, argc, script_argv);
              execve (script_argv[0], script_argv, envp);

              free (ptr);
            }
        }
    }
  else
    {
      size_t pathlen;
      size_t alloclen = 0;
      char *path = getenv ("PATH");
      if (path == NULL)
        {
          pathlen = confstr (_CS_PATH, (char *) NULL, 0);
          alloclen = pathlen + 1;
        }
      else
        pathlen = strlen (path);

      size_t len = strlen (file) + 1;
      alloclen += pathlen + len + 1;

      char *name;
      name = alloca (alloclen);

      if (path == NULL)
        {
          /* There is no `PATH' in the environment.
             The default search path is the current directory
             followed by the path `confstr' returns for `_CS_PATH'.  */
          path = name + pathlen + len + 1;
          path[0] = ':';
          (void) confstr (_CS_PATH, path + 1, pathlen);
        }

      /* Copy the file name at the top.  */
      name = (char *) memcpy (name + pathlen + 1, file, len);
      /* And add the slash.  */
      *--name = '/';

      char **script_argv = NULL;
      bool got_eacces = false;
      char *p = path;
      do
        {
          char *startp;

          path = p;
          p = strchr (path, ':');
          if (!p)
            p = path + strlen(path);

          if (p == path)
            /* Two adjacent colons, or a colon at the beginning or the end
               of `PATH' means to search the current directory.  */
            startp = name + 1;
          else
            startp = (char *) memcpy (name - (p - path), path, p - path);

          /* Try to execute this name.  If it works, execve will not return. */
          execve (startp, argv, envp);

          if (errno == ENOEXEC)
            {
              if (script_argv == NULL)
                {
                  /* Count the arguments.  */
                  int argc = 0;
                  while (argv[argc++])
                    ;
                  size_t arglen = (argc + 1) * sizeof (char *);
                  script_argv = alloca (arglen);
                  if (script_argv == NULL)
                    {
                      /* A possible EACCES error is not as important as
                         the ENOMEM.  */
                      got_eacces = false;
                      break;
                    }
                  scripts_argv (startp, argv, argc, script_argv);
                }

              execve (script_argv[0], script_argv, envp);
            }

          switch (errno)
            {
            case EACCES:
              /* Record the we got a `Permission denied' error.  If we end
                 up finding no executable we can use, we want to diagnose
                 that we did find one but were denied access.  */
              got_eacces = true;
            case ENOENT:
            case ESTALE:
            case ENOTDIR:
              /* Those errors indicate the file is missing or not executable
                 by us, in which case we want to just try the next path
                 directory.  */
            case ENODEV:
            case ETIMEDOUT:
              /* Some strange filesystems like AFS return even
                 stranger error numbers.  They cannot reasonably mean
                 anything else so ignore those, too.  */
              break;

            default:
              /* Some other error means we found an executable file, but
                 something went wrong executing it; return the error to our
                 caller.  */
              return -1;
            }
        }
      while (*p++ != '\0');

      /* We tried every element and none of them worked.  */
      if (got_eacces)
        /* At least one failure was due to permissions, so report that
           error.  */
        errno = EACCES;
    }

  /* Return the error from the last attempt (probably ENOENT).  */
  return -1;
}

#endif /* !defined(_MSC_VER) && !defined(HAVE_EXECVPE) */
