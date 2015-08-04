/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef CUSTOMVAROBJECT_H
#define CUSTOMVAROBJECT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/customvarobject.thpp"
#include "base/dynamicobject.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

enum ModifiedAttributeType
{
	ModAttrNotificationsEnabled = 1,
	ModAttrActiveChecksEnabled = 2,
	ModAttrPassiveChecksEnabled = 4,
	ModAttrEventHandlerEnabled = 8,
	ModAttrFlapDetectionEnabled = 16,
	ModAttrFailurePredictionEnabled = 32,
	ModAttrPerformanceDataEnabled = 64,
	ModAttrObsessiveHandlerEnabled = 128,
	ModAttrEventHandlerCommand = 256,
	ModAttrCheckCommand = 512,
	ModAttrNormalCheckInterval = 1024,
	ModAttrRetryCheckInterval = 2048,
	ModAttrMaxCheckAttempts = 4096,
	ModAttrFreshnessChecksEnabled = 8192,
	ModAttrCheckTimeperiod = 16384,
	ModAttrCustomVariable = 32768,
	ModAttrNotificationTimeperiod = 65536
};

/**
 * An object with custom variable attribute.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API CustomVarObject : public ObjectImpl<CustomVarObject>
{
public:
	DECLARE_OBJECT(CustomVarObject);

	virtual void ValidateVars(const Dictionary::Ptr& value, const ValidationUtils& utils) override;

	virtual int GetModifiedAttributes(void) const;
	virtual void SetModifiedAttributes(int flags, const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

	bool IsVarOverridden(const String& name) const;
};

}

#endif /* CUSTOMVAROBJECT_H */
