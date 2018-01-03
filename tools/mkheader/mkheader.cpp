/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/algorithm/string/replace.hpp>

int main(int argc, char **argv)
{
	if (argc < 4) {
		std::cerr << "Syntax: " << argv[0] << " <output-file> <fragments> [<file> ...]\n";
		return EXIT_FAILURE;
	}

	std::string tmpl = argv[2];
	size_t fragments = atoi(argv[1]);

	if (fragments <= 1)
		fragments = 1;

	std::vector<std::ofstream> fps{fragments};

	for (int i = 0; i < fps.size(); i++) {
		auto& fp = fps[i];

		std::string path{tmpl};
		boost::algorithm::replace_all(path, "{0}", std::to_string(i));

		fp.open(path.c_str(), std::ofstream::out);

		if (!fp) {
			std::cerr << "Failed to open \"" << path << "\".\n";
			return EXIT_FAILURE;
		}
	}

	size_t current_fragment = 0;

	for (int i = 3; i < argc; i++) {
		fps[current_fragment % fragments] << "#include \"" << argv[i] << "\"\n";
		current_fragment++;
	}

	return EXIT_SUCCESS;
}
