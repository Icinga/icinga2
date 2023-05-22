/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/table.hpp"
#include "base/dictionary.hpp"

namespace icinga
{


/**
 * @ingroup livestatus
 */
class HistoryTable : public Table
{
public:
	virtual void UpdateLogEntries(const Dictionary::Ptr& bag, int line_count, int lineno, const AddRowFunction& addRowFn) = 0;
};

}
