/*
 * popen_noshell: A faster implementation of popen() and system() for Linux.
 * Copyright (c) 2009 Ivan Zahariev (famzah)
 * Credits for the C++ test cases go to David Coz
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

#include "popen_noshell.h"
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

/*****************************************************
 * popen_noshell C++ unit test and use-case examples *
 *****************************************************
 *
 * Compile and test via:
 *	g++ -Wall popen_noshell.c popen_noshell_tests.cpp -o popen_noshell_tests_cpp && ./popen_noshell_tests_cpp
 */

#define DUMMY_SIZE 10000

// Just a dummy structure that allocates memory at startup and releases it 
// when exit() is called.

struct Dummy {
  Dummy() {
    val = new char[DUMMY_SIZE];
  }
  ~Dummy() {
    delete[] val;
    val = NULL;
  }
  char* val;
};

static Dummy dummy;

int main() {
  FILE *fp;
  char buf[256];
  int status;
  struct popen_noshell_pass_to_pclose pclose_arg;

  popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_CLONE);

  // We provide an invalid command, so that the child calls exit().
  // Child's exit() will result in destruction of global objects, while these
  // objects belong to the parent!
  // Therefore, if the parent uses them after child's exit(), it is likely to
  // lead to a crash.
  
  //char *exec_file = (char *) "ls";
  char *exec_file = (char *) "lsaaa";
  char *arg1 = (char *) "-la";
  char *arg2 = (char *) "/proc/self/fd";
  char *arg3 = (char *) NULL; /* last element */
  char *argv[] = {exec_file, arg1, arg2, arg3}; /* NOTE! The first argv[] must be the executed *exec_file itself */

  fp = popen_noshell(argv[0], (const char * const *)argv, "r", &pclose_arg, 0);
  if (!fp) {
    err(EXIT_FAILURE, "popen_noshell()");
  }

  while (fgets(buf, sizeof(buf)-1, fp)) {
    printf("Got line: %s", buf);
  }

  status = pclose_noshell(&pclose_arg);
  if (status == -1) {
    err(EXIT_FAILURE, "pclose_noshell()");
  } else {
    printf("The status of the child is %d. Note that this is not only the exit code. See man waitpid().\n", status);
  }

  // Trying to access our global variable stuff.
  // If exit() is used in the child process, dummy.val = 0 and we have a crash.
  // With _exit(), dummy.val is still valid.

  // printf("Accessing dummy stuff. dummy.val=%p\n", dummy.val);
  memset(dummy.val, 42, DUMMY_SIZE);
  printf("\nTests passed OK.\n");

  return 0;
}
