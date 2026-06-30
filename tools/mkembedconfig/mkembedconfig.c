// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *infp, *outfp;

	if (argc < 3) {
		fprintf(stderr, "Syntax: %s <in-file> <out-file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	infp = fopen(argv[1], "r");

	if (!infp) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	outfp = fopen(argv[2], "w");

	if (!outfp) {
		fclose(infp);
		perror("fopen");
		return EXIT_FAILURE;
	}

	fprintf(outfp, "/* This file has been automatically generated\n"
		"   from the input file \"%s\". */\n\n", argv[1]);
	fputs("#include \"config/configfragment.hpp\"\n\nnamespace {\n\nconst char *fragment = R\"CONFIG_FRAGMENT(", outfp);

	while (!feof(infp)) {
		char buf[1024];
		size_t rc = fread(buf, 1, sizeof(buf), infp);

		if (rc == 0)
			break;

		fwrite(buf, rc, 1, outfp);
	}

	fprintf(outfp, ")CONFIG_FRAGMENT\";\n\nREGISTER_CONFIG_FRAGMENT(\"%s\", fragment);\n\n}", argv[1]);

	fclose(outfp);
	fclose(infp);

	return EXIT_SUCCESS;
}
