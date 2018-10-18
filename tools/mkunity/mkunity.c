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
