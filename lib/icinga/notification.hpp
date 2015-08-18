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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "icinga/i2-icinga.hpp"
#include "icinga/notification.thpp"
#include "icinga/user.hpp"
#include "icinga/usergroup.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/checkresult.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "base/array.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
enum NotificationFilter
{
	StateFilterOK = 1,
	StateFilterWarning = 2,
	StateFilterCritical = 4,
	StateFilterUnknown = 8,

	StateFilterUp = 16,
	StateFilterDown = 32
};

/**
 * The notification type.
 *
 * @ingroup icinga
 */
enum NotificationType
{
	NotificationDowntimeStart = 0,
	NotificationDowntimeEnd = 1,
	NotificationDowntimeRemoved = 2,
	NotificationCustom = 3,
	NotificationAcknowledgement = 4,
	NotificationProblem = 5,
	NotificationRecovery = 6,
	NotificationFlappingStart = 7 ,
	NotificationFlappingEnd = 8,
};

class NotificationCommand;
class Checkable;
class ApplyRule;
struct ScriptFrame;
class Host;
class Service;

/**
 * An Icinga notification specification.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Notification : public ObjectImpl<Notification>
{
public:
	DECLARE_OBJECT(Notification);
	DECLARE_OBJECTNAME(Notification);

	static void StaticInitialize(void);

	intrusive_ptr<Checkable> GetCheckable(void) const;
	intrusive_ptr<NotificationCommand> GetCommand(void) const;
	TimePeriod::Ptr GetPeriod(void) const;
	std::set<User::Ptr> GetUsers(void) const;
	std::set<UserGroup::Ptr> GetUserGroups(void) const;

	void UpdateNotificationNumber(void);
	void ResetNotificationNumber(void);

	void BeginExecuteNotification(NotificationType type, const CheckResult::Ptr& cr, bool force, const String& author = "", const String& text = "");

	bool CheckNotificationUserFilters(NotificationType type, const User::Ptr& user, bool force);

	Endpoint::Ptr GetCommandEndpoint(void) const;

	static String NotificationTypeToString(NotificationType type);
	static String NotificationFilterToString(int filter);

	static boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> OnNextNotificationChanged;

	static void RegisterApplyRuleHandler(void);

	virtual void Validate(int types, const ValidationUtils& utils) override;

	virtual void ValidateStates(const Array::Ptr& value, const ValidationUtils& utils) override;
	virtual void ValidateTypes(const Array::Ptr& value, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

protected:
	virtual void OnConfigLoaded(void) override;
	virtual void OnAllConfigLoaded(void) override;
	virtual void Start(void) override;
	virtual void Stop(void) override;

private:
	void ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author = "", const String& text = "");

	static bool EvaluateApplyRuleInstance(const intrusive_ptr<Checkable>& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule);
	static bool EvaluateApplyRule(const intrusive_ptr<Checkable>& checkable, const ApplyRule& rule);

	static String NotificationTypeToStringInternal(NotificationType type);
	static String NotificationServiceStateToString(ServiceState state);
	static String NotificationHostStateToString(HostState state);
};

I2_ICINGA_API int ServiceStateToFilter(ServiceState state);
I2_ICINGA_API int HostStateToFilter(HostState state);
I2_ICINGA_API int FilterArrayToInt(const Array::Ptr& typeFilters, int defaultValue);
I2_ICINGA_API std::vector<String> FilterIntToArray(int iFilter);

}

#endif /* NOTIFICATION_H */
