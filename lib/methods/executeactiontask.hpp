/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXECUTEACTIONTASK_H
#define EXECUTEACTIONTASK_H

#include "methods/i2-methods.hpp"
#include "base/process.hpp"
#include "icinga/service.hpp"

namespace icinga
{

/**
 * Implements service checks based on external plugins.
 *
 * @ingroup methods
 */
class ExecuteActionTask
{
public:

	static void ProcessFinishedHandler(const Checkable::Ptr& service,
									   const CheckResult::Ptr& cr, const Value& commandLine, const ProcessResult& pr);
	static thread_local String ExecutionUUID;
private:

	ExecuteActionTask();
};

}

#endif /* EXECUTEACTIONTASK_H */
