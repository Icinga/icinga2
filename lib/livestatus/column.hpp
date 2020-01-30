/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef COLUMN_H
#define COLUMN_H

#include "base/value.hpp"
#include "livestatus/i2-livestatus.hpp"

using namespace icinga;

namespace icinga
{

enum LivestatusGroupByType {
	LivestatusGroupByNone,
	LivestatusGroupByHostGroup,
	LivestatusGroupByServiceGroup
};

class Column
{
public:
	typedef std::function<Value (const Value&)> ValueAccessor;
	typedef std::function<Value (const Value&, LivestatusGroupByType, const Object::Ptr&)> ObjectAccessor;

	Column(ValueAccessor valueAccessor, ObjectAccessor objectAccessor);

	Value ExtractValue(const Value& urow, LivestatusGroupByType groupByType = LivestatusGroupByNone, const Object::Ptr& groupByObject = Empty) const;

private:
	ValueAccessor m_ValueAccessor;
	ObjectAccessor m_ObjectAccessor;
};

}

#endif /* COLUMN_H */
