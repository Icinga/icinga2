// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "classcompiler.hpp"
#include <iostream>

using namespace icinga;

int main(int argc, char **argv)
{
	if (argc < 4) {
		std::cerr << "Syntax: " << argv[0] << " <input> <output-impl> <output-header>" << std::endl;
		return 1;
	}

	ClassCompiler::CompileFile(argv[1], argv[2], argv[3]);
}
