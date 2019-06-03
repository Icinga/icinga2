/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NOTIFICATIONRESULT_H
#define NOTIFICATIONRESULT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/notificationresult-ti.hpp"

namespace icinga
{

/**
 * A notification result.
 *
 * @ingroup icinga
 */
class NotificationResult final : public ObjectImpl<NotificationResult>
{
public:
	DECLARE_OBJECT(NotificationResult);

	double CalculateExecutionTime() const;
};

}

#endif /* NOTIFICATIONRESULT_H */
