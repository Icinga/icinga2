/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HISTORYTABLE_H
#define HISTORYTABLE_H

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
	virtual void UpdateLogEntries(const Dictionary::Ptr& bag, int lineno, const AddRowFunction& addRowFn) = 0;
};

}

#endif /* HISTORYTABLE_H */
