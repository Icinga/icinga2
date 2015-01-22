/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
	int cols;
	FILE *infp, *outfp;
	int i;
	char id[32];

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
		perror("fopen");
		return EXIT_FAILURE;
	}

	fprintf(outfp, "/* This file has been automatically generated\n"
	    "   from the input file \"%s\". */\n\n", argv[1]);
	fputs("#include \"config/configcompiler.hpp\"\n\nstatic const char g_ConfigFragment[] = {\n", outfp);
	fputc('\t', outfp);

	cols = 0;
	for (;;) {
		int c = fgetc(infp);

		if (c == EOF)
			break;

		if (cols > 16) {
			fputs("\n\t", outfp);
			cols = 0;
		}

		fprintf(outfp, "%d, ", c);
		cols++;
	}

	strncpy(id, argv[1], sizeof(id));
	id[sizeof(id) - 1] = '\0';

	for (i = 0; id[i]; i++) {
		if ((id[i] < 'a' || id[i] > 'z') && (id[i] < 'A' || id[i] > 'Z'))
			id[i] = '_';
	}

	fprintf(outfp, "0\n};\n\nREGISTER_CONFIG_FRAGMENT(%s, \"%s\", g_ConfigFragment);\n", id, argv[1]);

	fclose(outfp);
	fclose(infp);

	return EXIT_SUCCESS;
}
