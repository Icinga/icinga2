/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

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
