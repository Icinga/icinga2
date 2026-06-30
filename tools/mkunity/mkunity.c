// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	int i;

	if (argc < 3) {
		fprintf(stderr, "Syntax: %s <prefix> [<file> ...]\n", argv[0]);
		return EXIT_FAILURE;
	}

	for (i = 2; i < argc; i++) {
		printf("#include \"%s/%s\"\n", argv[1], argv[i]);
	}

	return EXIT_SUCCESS;
}
