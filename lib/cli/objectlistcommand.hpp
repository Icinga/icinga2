/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "cli/clicommand.hpp"
#include <ostream>

namespace icinga
{

/**
 * The "object list" command.
 *
 * @ingroup cli
 */
class ObjectListCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(ObjectListCommand);

	String GetDescription() const override;
	String GetShortDescription() const override;
	void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;
	int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;

private:
	static void PrintTypeCounts(std::ostream& fp, const std::map<String, int>& type_count);
};

}
