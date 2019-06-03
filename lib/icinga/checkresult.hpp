/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKRESULT_H
#define CHECKRESULT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkresult-ti.hpp"

namespace icinga
{

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
