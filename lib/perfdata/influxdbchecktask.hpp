/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#ifndef INFLUXDBCHECKTASK_H
#define INFLUXDBCHECKTASK_H

#include "perfdata/influxdbwriter.hpp"
#include "icinga/checkable.hpp"

namespace icinga {

/**
 * InfluDBWriter check type.
 *
 * @ingroup perfdata
 */
class InfluxdbCheckTask {
public:
	static void ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
		const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	InfluxdbCheckTask();
};

}

#endif /* INFLUXDBCHECKTASK_H */
