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
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "base/array.hpp"
#include <cstdint>

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
	StateFilterDown = 32,

	StateFilterAll = StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown | StateFilterUp | StateFilterDown,
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
	NotificationFlappingEnd = 256,

	NotificationTypeAll = NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved |
		NotificationCustom | NotificationAcknowledgement | NotificationProblem | NotificationRecovery |
		NotificationFlappingStart | NotificationFlappingEnd,
};

class NotificationCommand;
class ApplyRule;
struct ScriptFrame;
class Host;
class Service;
class UserGroup;

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

	Notification();

	static void StaticInitialize();

	intrusive_ptr<Checkable> GetCheckable() const;
	intrusive_ptr<NotificationCommand> GetCommand() const;
	TimePeriod::Ptr GetPeriod() const;
	std::set<User::Ptr> GetUsers() const;
	std::set<intrusive_ptr<UserGroup>> GetUserGroups() const;

	void UpdateNotificationNumber();
	void ResetNotificationNumber();

	void BeginExecuteNotification(NotificationType type, const CheckResult::Ptr& cr, bool force,
		bool reminder = false, const String& author = "", const String& text = "");

	Endpoint::Ptr GetCommandEndpoint() const;

	// Logging, etc.
	static String NotificationTypeToString(NotificationType type);
	// Compat, used for notifications, etc.
	static String NotificationTypeToStringCompat(NotificationType type);
	static String NotificationFilterToString(int filter, const std::map<String, int>& filterMap);

	static String NotificationServiceStateToString(ServiceState state);
	static String NotificationHostStateToString(HostState state);

	static boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> OnNextNotificationChanged;
	static boost::signals2::signal<void (const Notification::Ptr&, const String&, uint_fast8_t, const MessageOrigin::Ptr&)> OnLastNotifiedStatePerUserUpdated;
	static boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> OnLastNotifiedStatePerUserCleared;

	void Validate(int types, const ValidationUtils& utils) override;

	void ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateTimes(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

	static const std::map<String, int>& GetStateFilterMap();
	static const std::map<String, int>& GetTypeFilterMap();

	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

	Array::Ptr GetTypes() const override;
	void SetTypes(const Array::Ptr& value, bool suppress_events, const Value& cookie) override;

	Array::Ptr GetStates() const override;
	void SetStates(const Array::Ptr& value, bool suppress_events, const Value& cookie) override;

private:
	ObjectImpl<Checkable>::Ptr m_Checkable;
	// These attributes represent the actual notification "types" and "states" attributes from the "notification.ti".
	// However, since we want to ensure that the type and state bitsets are always in sync with those attributes,
	// we need to override their setters, and this on the hand introduces another problem: The virtual setters are
	// called from within the ObjectImpl<Notification> constructor, which obviously violates the C++ standard [^1].
	// So, in order to avoid all this kind of mess, these two attributes have the "no_storage" flag set, and
	// their getters/setters are pure virtual, which means this class has to provide the implementation of them.
	//
	// [^1]: https://isocpp.org/wiki/faq/strange-inheritance#calling-virtuals-from-ctors
	AtomicOrLocked<Array::Ptr> m_Types;
	AtomicOrLocked<Array::Ptr> m_States;

	bool CheckNotificationUserFilters(NotificationType type, const User::Ptr& user, bool force, bool reminder);

	void ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, const String& author = "", const String& text = "");

	static bool EvaluateApplyRuleInstance(const intrusive_ptr<Checkable>& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule, bool skipFilter);
	static bool EvaluateApplyRule(const intrusive_ptr<Checkable>& checkable, const ApplyRule& rule, bool skipFilter = false);

	static std::map<String, int> m_StateFilterMap;
	static std::map<String, int> m_TypeFilterMap;
};

int ServiceStateToFilter(ServiceState state);
int HostStateToFilter(HostState state);

}

#endif /* NOTIFICATION_H */
