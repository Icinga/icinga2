/*
 * popen_noshell: A faster implementation of popen() and system() for Linux.
 * Copyright (c) 2009 Ivan Zahariev (famzah)
 * Copyright (c) 2012 Gunnar Beutner
 * Version: 1.0
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#ifndef POPEN_NOSHELL_H
#define POPEN_NOSHELL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

struct popen_noshell_pass_to_pclose {
	FILE *fp;
	pid_t pid;
};

/***************************
 * PUBLIC FUNCTIONS FOLLOW *
 ***************************/

/* this is the native function call */
FILE *popen_noshell(const char *file, const char * const *argv, const char *type, struct popen_noshell_pass_to_pclose *pclose_arg, int ignore_stderr);

/* more insecure, but more compatible with popen() */
FILE *popen_noshell_compat(const char *command, const char *type, struct popen_noshell_pass_to_pclose *pclose_arg);

/* call this when you have finished reading and writing from/to the child process */
int pclose_noshell(struct popen_noshell_pass_to_pclose *arg); /* the pclose() equivalent */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
