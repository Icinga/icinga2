// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VARIABLEUTILITY_H
#define VARIABLEUTILITY_H

#include "base/i2-base.hpp"
#include "cli/i2-cli.hpp"
#include "base/dictionary.hpp"
#include "base/string.hpp"
#include <ostream>

namespace icinga
{

/**
 * @ingroup cli
 */
class VariableUtility
{
public:
	static Value GetVariable(const String& name);
	static void PrintVariables(std::ostream& outfp);

private:
	VariableUtility();

};

}

#endif /* VARIABLEUTILITY_H */
