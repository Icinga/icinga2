/*
 * popen_noshell: A faster implementation of popen() and system() for Linux.
 * Copyright (c) 2009 Ivan Zahariev (famzah)
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

/***********************************
 * popen_noshell use-case examples *
 ***********************************
 *
 * Compile and test via:
 * 	gcc -Wall popen_noshell.c popen_noshell_examples.c -o popen_noshell_examples && ./popen_noshell_examples
 *
 * If you want to profile using Valgrind, then compile and run via:
 *	gcc -Wall -g -DPOPEN_NOSHELL_VALGRIND_DEBUG popen_noshell.c popen_noshell_examples.c -o popen_noshell_examples && \
 *        valgrind -q --tool=memcheck --leak-check=yes --show-reachable=yes --track-fds=yes ./popen_noshell_examples
 */

void satisfy_open_FDs_leak_detection_and_exit() {
	/* satisfy Valgrind FDs leak detection for the parent process */
	if (fflush(stdout) != 0) err(EXIT_FAILURE, "fflush(stdout)");
	if (fflush(stderr) != 0) err(EXIT_FAILURE, "fflush(stderr)");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	exit(0);
}

void example_reading(int use_compat) {
	FILE *fp;
	char buf[256];
	int status;
	struct popen_noshell_pass_to_pclose pclose_arg;

	char *cmd = "ls -la /proc/self/fd"; /* used only by popen_noshell_compat(), we discourage this type of providing a command */

	/* the command arguments used by popen_noshell() */
	char *exec_file = "ls";
	char *arg1 = "-la";
	char *arg2 = "/proc/self/fd";
	char *arg3 = (char *) NULL; /* last element */
	char *argv[] = {exec_file, arg1, arg2, arg3}; /* NOTE! The first argv[] must be the executed *exec_file itself */
	
	if (use_compat) {
		fp = popen_noshell_compat(cmd, "r", &pclose_arg);
		if (!fp) {
			err(EXIT_FAILURE, "popen_noshell_compat()");
		}
	} else {
		fp = popen_noshell(exec_file, (const char * const *)argv, "r", &pclose_arg, 0);
		if (!fp) {
			err(EXIT_FAILURE, "popen_noshell()");
		}
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
}

void example_writing(int use_compat) {
	FILE *fp;
	int status;
	struct popen_noshell_pass_to_pclose pclose_arg;

	char *cmd = "tee -a /tmp/popen-noshell.txt"; /* used only by popen_noshell_compat(), we discourage this type of providing a command */

	/* the command arguments used by popen_noshell() */
	char *exec_file = "tee";
	char *arg1 = "-a";
	char *arg2 = "/tmp/popen-noshell.txt";
	char *arg3 = (char *) NULL; /* last element */
	char *argv[] = {exec_file, arg1, arg2, arg3}; /* NOTE! The first argv[] must be the executed *exec_file itself */
	
	if (use_compat) {
		fp = popen_noshell_compat(cmd, "w", &pclose_arg);
		if (!fp) {
			err(EXIT_FAILURE, "popen_noshell_compat()");
		}
	} else {
		fp = popen_noshell(exec_file, (const char * const *)argv, "w", &pclose_arg, 0);
		if (!fp) {
			err(EXIT_FAILURE, "popen_noshell()");
		}
	}

	if (fprintf(fp, "This is a test line, my pid is %d\n", (int)getpid()) < 0) {
		err(EXIT_FAILURE, "fprintf()");
	}

	status = pclose_noshell(&pclose_arg);
	if (status == -1) {
		err(EXIT_FAILURE, "pclose_noshell()");
	} else {
		printf("The status of the child is %d. Note that this is not only the exit code. See man waitpid().\n", status);
	}
	
	printf("Done, you can see the results by executing: cat %s\n", arg2);
}

int main() {
	int try_compat;
	int try_read;

	/*
	 * Tune these options as you need.
	 */
	try_compat = 0;	/* or the more secure, but incompatible version of popen_noshell */
	try_read = 1;	/* or write */

	if (try_read) {
		example_reading(try_compat);
	} else {
		example_writing(try_compat);
	}

	satisfy_open_FDs_leak_detection_and_exit();

	return 0;
}
