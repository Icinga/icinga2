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

/***************************************************
 * popen_noshell C unit test and use-case examples *
 ***************************************************
 *
 * Compile and test via:
 * 	gcc -Wall popen_noshell.c popen_noshell_tests.c -o popen_noshell_tests && ./popen_noshell_tests
 *
 * Compile for debugging by Valgrind via:
 * 	gcc -Wall -g -DPOPEN_NOSHELL_VALGRIND_DEBUG popen_noshell.c popen_noshell_tests.c -o popen_noshell_tests
 * Then start under Valgrind via:
 * 	valgrind -q --tool=memcheck --leak-check=yes --show-reachable=yes --track-fds=yes ./popen_noshell_tests
 * If you want to filter Valgrind false reports about 0 opened file descriptors, add the following at the end:
 *	2>&1|egrep -v '^==[[:digit:]]{1,5}==( | FILE DESCRIPTORS: 0 open at exit.)$'
 */

int do_unit_tests_ignore_stderr;

void satisfy_open_FDs_leak_detection_and_exit() {
	/* satisfy Valgrind FDs leak detection for the parent process */
	if (fflush(stdout) != 0) err(EXIT_FAILURE, "fflush(stdout)");
	if (fflush(stderr) != 0) err(EXIT_FAILURE, "fflush(stderr)");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	exit(0);
}

void assert_string(char *expected, char *got, const char *assert_desc) {
	if (strcmp(expected, got) != 0) errx(EXIT_FAILURE, "%s: Expected '%s', got '%s'", assert_desc, expected, got);
}

void assert_int(int expected, int got, const char *assert_desc) {
	if (expected != got) errx(EXIT_FAILURE, "%s: Expected %d, got %d", assert_desc, expected, got);
}

void assert_status_not_internal_error(int status) {
	assert_int(1, status >= 0, "assert_status_not_internal_error");
}

void assert_status_signal(int signal, int status) {
	assert_status_not_internal_error(status);
	assert_int(signal, status & 127, "assert_status_signal");
}

void assert_status_exit_code(int code, int status) {
	assert_status_not_internal_error(status);
	assert_status_signal(0, status);
	assert_int(code, status >> 8, "assert_status_exit_code");
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

void unit_test(int reading, char *argv[], char *expected_string, int expected_signal, int expected_exit_code) {
	FILE *fp;
	char buf[256];
	int status;
	struct popen_noshell_pass_to_pclose pclose_arg;
	char *received;
	size_t received_size;

	fp = popen_noshell(argv[0], (const char * const *)argv, reading ? "r" : "w", &pclose_arg, do_unit_tests_ignore_stderr);
	if (!fp) err(EXIT_FAILURE, "popen_noshell");

	if (reading) {
		received_size = strlen(expected_string) + 256; // so that we can store a bit longer strings that we expected and discover the mismatch
		received = alloca(received_size); // use alloca() or else the fork()'ed child will generate a Valgrind memory leak warning if exec() fails
		if (!received) err(EXIT_FAILURE, "alloca");
		memset(received, 0, received_size); // ensure a terminating null

		while (fgets(buf, sizeof(buf) - 1, fp)) {
			strncat(received, buf, received_size - strlen(received) - 2);
		}

		assert_string(expected_string, received, "Received string");
	}

	status = pclose_noshell(&pclose_arg);
	if (status == -1) {
		err(EXIT_FAILURE, "pclose_noshell()");
	} else {
		if (expected_signal != 0) {
			assert_status_signal(expected_signal, status);
		} else {
			assert_status_exit_code(expected_exit_code, status);
		}
	}

	//free(received); // memory allocated by alloca() cannot be free()'d
}

void do_unit_tests() {
	int test_num = 0;
	int more_to_test = 1;
	char *bin_bash = "/bin/bash";
	char *bin_true = "/bin/true";
	char *bin_cat  = "/bin/cat";
	char *bin_echo = "/bin/echo";

	do {
		++test_num;
		switch (test_num) {
			case 1: {
				char *argv[] = {"/", NULL};
				unit_test(1, argv, "", 0, 255); // failed to execute binary (status code is -1, STDOUT is empty, STDERR text)
				break;
			}
			case 2: {
				char *argv[] = {bin_bash, "-c", "ulimit -t 1 && while [ 1 ]; do let COUNTER=COUNTER+1; done;", NULL};
				unit_test(1, argv, "", 9, 5); // process signalled with 9 due to CPU limit (STDOUT/ERR are empty)
				break;
			}
			case 3: {
				char *argv[] = {bin_bash, "-c", "sleep 1; exit 1", NULL};
				unit_test(1, argv, "", 0, 1); // process exited with value 1   (STDOUT/ERR are empty)
				break;
			}
			case 4: {
				char *argv[] = {bin_bash, "-c", "exit 255", NULL};
				unit_test(1, argv, "", 0, 255); // process exited with value 255 (STDOUT/ERR are empty)
				break;
			}
			case 5: {
				char *argv[] = {bin_bash, "-c", "echo \"some err string\" 1>&2; exit 111", NULL};
				unit_test(1, argv, "", 0, 111); // process exited with value 111 (STDERR text, STDOUT is empty)
				break;
			}
			case 6: {
				char *argv[] = {bin_bash, "-c", "echo -en \"some err\\nstring v2\" 1>&2; echo -en \"some\\ngood text\"; exit 0", NULL};
				unit_test(1, argv, "some\ngood text", 0, 0); // process exited with value 0 (STDERR text, STDOUT text)
				break;
			}
			case 7: {
				char *argv[] = {bin_bash, "-c", "echo -e \"\" 1>&2; echo -e \"\"; exit 3", NULL};
				unit_test(1, argv, "\n", 0, 3); // process exited with value 3 (STDERR text, STDOUT text)
				break;
			}
			case 8: {
				char *argv[] = {bin_bash, NULL};
				unit_test(1, argv, "", 0, 0); // process exited with value 0 (single argument, STDOUT/ERR are empty)
				break;
			}
			case 9: {
				char *argv[] = {bin_true, NULL};
				unit_test(1, argv, "", 0, 0); // process exited with value 0 (single argument, STDOUT/ERR are empty)
				break;
			}
			case 10: {
				char *argv[] = {bin_cat, NULL}; // cat expects an input from STDIN
				unit_test(1, argv, "", 0, 0); // process exited with value 0 (single argument, STDOUT/ERR are empty)
				break;
			}
			case 11: {
				char *argv[] = {bin_echo, NULL};
				unit_test(1, argv, "\n", 0, 0); // process exited with value 0 (single argument, STDERR is empty, STDOUT text)
				break;
			}
			default:
				more_to_test = 0;
				break;
		}
	} while (more_to_test);

	assert_int(11, test_num - 1, "Test count");
}

void proceed_to_unit_tests_and_exit() {
	popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_CLONE); /* the default one */
	do_unit_tests();
	popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_FORK);
	do_unit_tests();

	printf("Tests passed OK.\n");

	satisfy_open_FDs_leak_detection_and_exit();
}

int main() {
	do_unit_tests_ignore_stderr = 1; /* do we ignore STDERR from the executed commands? */
	proceed_to_unit_tests_and_exit();
	satisfy_open_FDs_leak_detection_and_exit();

	return 0;
}
