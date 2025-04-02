/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/object.hpp"

namespace icinga
{

/**
 * A component that produces check results and hence is responsible for them, e.g. CheckerComponent.
 * I.e. on its shutdown it has to clean up and/or wait for its own ongoing checks if any.
 *
 * @ingroup icinga
 */
class CheckResultProducer : virtual public Object
{
public:
	DECLARE_PTR_TYPEDEFS(CheckResultProducer);

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

}
