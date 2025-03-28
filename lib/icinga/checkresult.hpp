/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKRESULT_H
#define CHECKRESULT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkresult-ti.hpp"

namespace icinga
{

/**
 * A component that produces check results and hence is responsible for them, e.g. CheckerComponent.
 * I.e. on its shutdown it has to clean up and/or wait for its own ongoing checks if any.
 *
 * @ingroup icinga
 */
class CheckResultProducer
{
public:
	/**
	 * Requests to delay the producer shutdown (if any) for a CheckResult to be processed.
	 *
	 * @return Whether request was accepted.
	 */
	virtual bool try_lock_shared() noexcept = 0;

	/**
	 * Releases one semaphore slot acquired for CheckResult processing.
	 */
	virtual void unlock_shared() noexcept = 0;
};

/**
 * A check result.
 *
 * @ingroup icinga
 */
class CheckResult final : public ObjectImpl<CheckResult>
{
public:
	DECLARE_OBJECT(CheckResult);

	double CalculateExecutionTime() const;
	double CalculateLatency() const;
};

}

#endif /* CHECKRESULT_H */
