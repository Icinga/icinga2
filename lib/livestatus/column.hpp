/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/i2-livestatus.hpp"
#include "base/value.hpp"

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
