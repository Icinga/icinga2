/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-cib.h"

using namespace icinga;

bool ServiceStatusMessage::GetService(string *service) const
{
	return Get("service", service);
}

void ServiceStatusMessage::SetService(const string& service)
{
	Set("service", service);
}

bool ServiceStatusMessage::GetState(ServiceState *state) const
{
	long value;
	if (Get("state", &value)) {
		*state = static_cast<ServiceState>(value);
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetState(ServiceState state)
{
	Set("state", static_cast<long>(state));
}

bool ServiceStatusMessage::GetStateType(ServiceStateType *type) const
{
	long value;
	if (Get("state_type", &value)) {
		*type = static_cast<ServiceStateType>(value);
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetStateType(ServiceStateType type)
{
	Set("state_type", static_cast<long>(type));
}

bool ServiceStatusMessage::GetCurrentCheckAttempt(long *attempt) const
{
	return Get("current_attempt", attempt);
}

void ServiceStatusMessage::SetCurrentCheckAttempt(long attempt)
{
	Set("current_attempt", attempt);
}

bool ServiceStatusMessage::GetCheckResult(CheckResult *cr) const
{
	Dictionary::Ptr obj;
	if (Get("result", &obj)) {
		*cr = CheckResult(MessagePart(obj));
		return true;
	}
	return false;
}

void ServiceStatusMessage::SetCheckResult(CheckResult cr)
{
	Set("result", cr.GetDictionary());
}
