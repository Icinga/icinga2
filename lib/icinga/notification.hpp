/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "icinga/checkable.thpp"
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
	NotificationDowntimeStart = 1,
	NotificationDowntimeEnd = 2,
	NotificationDowntimeRemoved = 4,
	NotificationCustom = 8,
	NotificationAcknowledgement = 16,
	NotificationProblem = 32,
	NotificationRecovery = 64,
	NotificationFlappingStart = 128,
	NotificationFlappingEnd = 256
};

class NotificationCommand;
class ApplyRule;
struct ScriptFrame;
class Host;
class Service;

/**
 * An Icinga notification specification.
 *
 * @ingroup icinga
 */
class Notification final : public ObjectImpl<Notification>
{
public:
	DECLARE_OBJECT(Notification);
	DECLARE_OBJECTNAME(Notification);

	static void StaticInitialize();

	intrusive_ptr<Checkable> GetCheckable() const;
	intrusive_ptr<NotificationCommand> GetCommand() const;
	TimePeriod::Ptr GetPeriod() const;
	std::set<User::Ptr> GetUsers() const;
	std::set<UserGroup::Ptr> GetUserGroups() const;

	void UpdateNotificationNumber();
	void ResetNotificationNumber();

	void BeginExecuteNotification(NotificationType type, const CheckResult::Ptr& cr, bool force,
		bool reminder = false, const String& author = "", const String& text = "");

	Endpoint::Ptr GetCommandEndpoint() const;

	static String NotificationTypeToString(NotificationType type);
	static String NotificationFilterToString(int filter, const std::map<String, int>& filterMap);

	static boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> OnNextNotificationChanged;

	virtual void Validate(int types, const ValidationUtils& utils) override;

	virtual void ValidateStates(const Array::Ptr& value, const ValidationUtils& utils) override;
	virtual void ValidateTypes(const Array::Ptr& value, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

	static const std::map<String, int>& GetStateFilterMap();
	static const std::map<String, int>& GetTypeFilterMap();

protected:
	virtual void OnConfigLoaded() override;
	virtual void OnAllConfigLoaded() override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

private:
	ObjectImpl<Checkable>::Ptr m_Checkable;

	bool CheckNotificationUserFilters(NotificationType type, const User::Ptr& user, bool force, bool reminder);

	void ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author = "", const String& text = "");

	static bool EvaluateApplyRuleInstance(const intrusive_ptr<Checkable>& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule);
	static bool EvaluateApplyRule(const intrusive_ptr<Checkable>& checkable, const ApplyRule& rule);

	static String NotificationTypeToStringInternal(NotificationType type);
	static String NotificationServiceStateToString(ServiceState state);
	static String NotificationHostStateToString(HostState state);

	static std::map<String, int> m_StateFilterMap;
	static std::map<String, int> m_TypeFilterMap;
};

int ServiceStateToFilter(ServiceState state);
int HostStateToFilter(HostState state);

}

#endif /* NOTIFICATION_H */
