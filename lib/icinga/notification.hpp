/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "icinga/i2-icinga.hpp"
#include "icinga/notification-ti.hpp"
#include "icinga/checkable-ti.hpp"
#include "icinga/user.hpp"
#include "icinga/usergroup.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/checkresult.hpp"
#include "icinga/notificationresult.hpp"
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

	void ProcessNotificationResult(const NotificationResult::Ptr& nr, const MessageOrigin::Ptr& origin = nullptr);

	static String NotificationTypeToString(NotificationType type);
	static String NotificationFilterToString(int filter, const std::map<String, int>& filterMap);

	static boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> OnNextNotificationChanged;
	static boost::signals2::signal<void (const Notification::Ptr&, const NotificationResult::Ptr&, const MessageOrigin::Ptr&)> OnNewNotificationResult;

	void Validate(int types, const ValidationUtils& utils) override;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateTimes(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

	static const std::map<String, int>& GetStateFilterMap();
	static const std::map<String, int>& GetTypeFilterMap();

protected:
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

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
