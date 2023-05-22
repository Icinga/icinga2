/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
