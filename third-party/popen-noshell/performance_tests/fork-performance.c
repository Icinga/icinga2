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

#include <unistd.h>
#include <err.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include "popen_noshell.h"

/*
 * This is a performance test program.
 * It invokes the extremely light, statically build binary "./tiny2" which outputs "Hello, world!" and exits.
 *
 * Different approaches for calling "./tiny2" are tried, in order to compare their performance results.
 *
 */

#define USE_LIBC_POPEN 0
#define USE_NOSHELL_POPEN 1

int use_noshell_compat = 0;

void popen_test(int type) {
	char *exec_file = "./tiny2";
	char *arg1 = (char *) NULL;
	char *argv[] = {exec_file, arg1};
	FILE *fp;
	struct popen_noshell_pass_to_pclose pclose_arg;
	int status;
	char buf[64];

	if (type) {
		if (!use_noshell_compat) {
			fp = popen_noshell(exec_file, (const char * const *)argv, "r", &pclose_arg, 0);
		} else {
			fp = popen_noshell_compat(exec_file, "r", &pclose_arg);
			argv[0] = NULL; // satisfy GCC warnings
		}
	} else {
		fp = popen("./tiny2", "r");
	}
	if (!fp) {
		err(EXIT_FAILURE, "popen()");
	}

	while (fgets(buf, sizeof(buf)-1, fp)) {
		if (strcmp(buf, "Hello, world!\n") != 0) {
			errx(EXIT_FAILURE, "bad response: %s", buf);
		}
	}

	if (type) {
		status = pclose_noshell(&pclose_arg);
	} else {
		status = pclose(fp);
	}
	if (status == -1) {
		err(EXIT_FAILURE, "pclose()");
	}
	if (status != 0) {
		errx(EXIT_FAILURE, "status code is non-zero");
	}
}

void fork_test(int type) {
	pid_t pid;
	int status;

	if (type) {
		pid = fork();
	} else {
		pid = vfork();
	}
	if (pid == -1) {
		err(EXIT_FAILURE, "fork()");
	}

	if (pid == 0) { // child
		execl("./tiny2", "./tiny2", (char *) NULL);
		_exit(255);
	}

	// parent process
	if (waitpid(pid, &status, 0) != pid) {
		err(EXIT_FAILURE, "waitpid()");
	}
	if (status != 0) {
		errx(EXIT_FAILURE, "status code is non-zero");
	}
}

char *allocate_memory(int size_in_mb, int ratio) {
	char *m;
	int size;
	int i;

	size = size_in_mb*1024*1024;
	m = malloc(sizeof(char) * size);
	if (!m) {
		err(EXIT_FAILURE, "malloc()");
	}

	/* allocate part of the memory, so that we can simulate some memory activity before fork()'ing */
	if (ratio != 0) {
		for (i = 0; i < size/ratio; ++i) {
			*(m + i) = 'z';
		}
	}

	return m;
}

int safe_atoi(char *s) {
	int i;

	if (strlen(s) == 0) {
		errx(EXIT_FAILURE, "safe_atoi(): String is empty");
	}

	for (i = 0; i < strlen(s); ++i) {
		if (!isdigit(s[i])) {
			errx(EXIT_FAILURE, "safe_atoi(): Non-numeric characters found in string '%s'", s);
		}
	}

	return atoi(s);
}

void parse_argv(int argc, char **argv, int *count, int *allocated_memory_size_in_mb, int *allocated_memory_usage_ratio, int *test_mode) {
	const struct option long_options[] = {
		{"count", 1, 0, 1},
		{"memsize", 1, 0, 2},
		{"ratio", 1, 0, 3},
		{"mode", 1, 0, 4},
		{0, 0, 0, 0}
	};
	int c;
	int usage = 0;
	int got[4];
	int optarg_int;

	memset(&got, 0, sizeof(got));
	while (1) {
		c = getopt_long(argc, argv, "", &long_options[0], NULL);
		if (c == -1) { // no more arguments
			break;
		}

		if (c >= 1 && c <= sizeof(got)/sizeof(int)) {
			got[c-1] = 1;
		}

		if (!optarg) {
			warnx("You provided no value");
			usage = 1;
			break;
		}
		optarg_int = safe_atoi(optarg);

		switch (c) {
			case 1:
				*count = optarg_int;
				break;
			case 2:
				*allocated_memory_size_in_mb = optarg_int;
				break;
			case 3:
				*allocated_memory_usage_ratio = optarg_int;
				break;
			case 4:
				*test_mode = optarg_int;
				break;
			default:
				warnx("Bad option");
				usage = 1;
				break;
		}

		if (usage) {
			break;
		}
	}

	for (c = 0; c < sizeof(got)/sizeof(int); ++c) {
		if (!got[c]) {
			warnx("Option #%d not specified", c);
			usage = 1;
		}
	}

	if (usage) {
		warnx("Usage: %s ...options - all are required...\n", argv[0]);
		warnx("\t--count\n\t--memsize [MBytes]\n\t--ratio [0..N, 0=no_usage_of_memory]\n\t--mode [0..7]\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv) {
	int count;
	char *m;
	int allocated_memory_size_in_mb;
	int allocated_memory_usage_ratio;
	int test_mode;
	int wrote = 0;

	count = 30000;
	allocated_memory_size_in_mb = 20;
	allocated_memory_usage_ratio = 2; /* the memory usage is 1 divided by the "allocated_memory_usage_ratio", use 0 for "no usage" at all */
	test_mode = 5;

	parse_argv(argc, argv, &count, &allocated_memory_size_in_mb, &allocated_memory_usage_ratio, &test_mode);

	m = allocate_memory(allocated_memory_size_in_mb, allocated_memory_usage_ratio);

	warnx("Test options: count=%d, memsize=%d, ratio=%d, mode=%d", count, allocated_memory_size_in_mb, allocated_memory_usage_ratio, test_mode);

	while (count--) {
		switch (test_mode) {
			/* the following fork + exec calls do not return the output of their commands */
			case 0:
				if (!wrote) warnx("fork() + exec(), standard Libc");
				fork_test(1);
				break;
			case 1:
				if (!wrote) warnx("vfork() + exec(), standard Libc");
				fork_test(0);
				break;
			case 2:
				if (!wrote) warnx("system(), standard Libc");
				system("./tiny2 >/dev/null");
				break;

			/* all the below popen() use-cases are tested if they return the correct string in *fp */
			case 3:
				if (!wrote) warnx("popen(), standard Libc");
				popen_test(USE_LIBC_POPEN);
				break;
			case 4:
				use_noshell_compat = 0;
				if (!wrote) warnx("the new noshell, debug fork(), compat=%d", use_noshell_compat);
				popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_FORK);
				popen_test(USE_NOSHELL_POPEN);
				break;
			case 5:
				use_noshell_compat = 0;
				if (!wrote) warnx("the new noshell, default vfork(), compat=%d", use_noshell_compat);
				popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_CLONE);
				popen_test(USE_NOSHELL_POPEN);
				break;
			case 6:
				use_noshell_compat = 1;
				if (!wrote) warnx("the new noshell, debug fork(), compat=%d", use_noshell_compat);
				popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_FORK);
				popen_test(USE_NOSHELL_POPEN);
				break;
			case 7:
				use_noshell_compat = 1;
				if (!wrote) warnx("the new noshell, default vfork(), compat=%d", use_noshell_compat);
				popen_noshell_set_fork_mode(POPEN_NOSHELL_MODE_CLONE);
				popen_test(USE_NOSHELL_POPEN);
				break;
			default:
				errx(EXIT_FAILURE, "Bad mode");
				break;
		}
		wrote = 1;
	}

	return 0;
}
